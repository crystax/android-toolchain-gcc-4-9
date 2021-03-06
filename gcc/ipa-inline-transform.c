/* Callgraph transformations to handle inlining
   Copyright (C) 2003-2014 Free Software Foundation, Inc.
   Contributed by Jan Hubicka

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* The inline decisions are stored in callgraph in "inline plan" and
   applied later.

   To mark given call inline, use inline_call function.
   The function marks the edge inlinable and, if necessary, produces
   virtual clone in the callgraph representing the new copy of callee's
   function body.

   The inline plan is applied on given function body by inline_transform.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "dumpfile.h"
#include "tm.h"
#include "tree.h"
#include "langhooks.h"
#include "intl.h"
#include "coverage.h"
#include "ggc.h"
#include "tree-cfg.h"
#include "ipa-prop.h"
#include "ipa-inline.h"
#include "tree-inline.h"
#include "tree-pass.h"
#include "l-ipo.h"
#include "params.h"
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "gimple-expr.h"
#include "gimple.h"

int ncalls_inlined;
int nfunctions_inlined;
bool speculation_removed;

/* Scale frequency of NODE edges by FREQ_SCALE.  */

static void
update_noncloned_frequencies (struct cgraph_node *node,
			      int freq_scale)
{
  struct cgraph_edge *e;

  /* We do not want to ignore high loop nest after freq drops to 0.  */
  if (!freq_scale)
    freq_scale = 1;
  for (e = node->callees; e; e = e->next_callee)
    {
      e->frequency = e->frequency * (gcov_type) freq_scale / CGRAPH_FREQ_BASE;
      if (e->frequency > CGRAPH_FREQ_MAX)
        e->frequency = CGRAPH_FREQ_MAX;
      if (!e->inline_failed)
        update_noncloned_frequencies (e->callee, freq_scale);
    }
  for (e = node->indirect_calls; e; e = e->next_callee)
    {
      e->frequency = e->frequency * (gcov_type) freq_scale / CGRAPH_FREQ_BASE;
      if (e->frequency > CGRAPH_FREQ_MAX)
        e->frequency = CGRAPH_FREQ_MAX;
    }
}

/* We removed or are going to remove the last call to NODE.
   Return true if we can and want proactively remove the NODE now.
   This is important to do, since we want inliner to know when offline
   copy of function was removed.  */

static bool
can_remove_node_now_p_1 (struct cgraph_node *node)
{
  /* FIXME: When address is taken of DECL_EXTERNAL function we still
     can remove its offline copy, but we would need to keep unanalyzed node in
     the callgraph so references can point to it.  */
  return (!node->address_taken
	  && !ipa_ref_has_aliases_p (&node->ref_list)
	  && cgraph_can_remove_if_no_direct_calls_p (node)
	  /* Inlining might enable more devirtualizing, so we want to remove
	     those only after all devirtualizable virtual calls are processed.
	     Lacking may edges in callgraph we just preserve them post
	     inlining.  */
	  && !DECL_VIRTUAL_P (node->decl)
	  /* During early inlining some unanalyzed cgraph nodes might be in the
	     callgraph and they might reffer the function in question.  */
	  && !cgraph_new_nodes);
}

/* We are going to eliminate last direct call to NODE (or alias of it) via edge E.
   Verify that the NODE can be removed from unit and if it is contained in comdat
   group that the whole comdat group is removable.  */

static bool
can_remove_node_now_p (struct cgraph_node *node, struct cgraph_edge *e)
{
  struct cgraph_node *next;
  if (!can_remove_node_now_p_1 (node))
    return false;

  /* When we see same comdat group, we need to be sure that all
     items can be removed.  */
  if (!node->same_comdat_group)
    return true;
  for (next = cgraph (node->same_comdat_group);
       next != node; next = cgraph (next->same_comdat_group))
    if ((next->callers && next->callers != e)
	|| !can_remove_node_now_p_1 (next))
      return false;
  return true;
}


/* E is expected to be an edge being inlined.  Clone destination node of
   the edge and redirect it to the new clone.
   DUPLICATE is used for bookkeeping on whether we are actually creating new
   clones or re-using node originally representing out-of-line function call.
   By default the offline copy is removed, when it appears dead after inlining.
   UPDATE_ORIGINAL prevents this transformation.
   If OVERALL_SIZE is non-NULL, the size is updated to reflect the
   transformation.
   FREQ_SCALE specify the scaling of frequencies of call sites.  */

void
clone_inlined_nodes (struct cgraph_edge *e, bool duplicate,
		     bool update_original, int *overall_size, int freq_scale)
{
  struct cgraph_node *inlining_into;
  struct cgraph_edge *next;

  if (e->caller->global.inlined_to)
    inlining_into = e->caller->global.inlined_to;
  else
    inlining_into = e->caller;

  if (duplicate)
    {
      /* We may eliminate the need for out-of-line copy to be output.
	 In that case just go ahead and re-use it.  This is not just an
	 memory optimization.  Making offline copy of fuction disappear
	 from the program will improve future decisions on inlining.  */
      if (!e->callee->callers->next_caller
	  /* Recursive inlining never wants the master clone to
	     be overwritten.  */
	  && update_original
	  && can_remove_node_now_p (e->callee, e))
	{
	  /* TODO: When callee is in a comdat group, we could remove all of it,
	     including all inline clones inlined into it.  That would however
	     need small function inlining to register edge removal hook to
	     maintain the priority queue.

	     For now we keep the ohter functions in the group in program until
	     cgraph_remove_unreachable_functions gets rid of them.  */
	  gcc_assert (!e->callee->global.inlined_to);
          symtab_dissolve_same_comdat_group_list (e->callee);
	  if (e->callee->definition && !DECL_EXTERNAL (e->callee->decl))
	    {
	      if (overall_size)
	        *overall_size -= inline_summary (e->callee)->size;
	      nfunctions_inlined++;
	    }
	  duplicate = false;
	  e->callee->externally_visible = false;
          update_noncloned_frequencies (e->callee, e->frequency);
	}
      else
	{
	  struct cgraph_node *n;
	  /* Disable updating of callee count if caller is not the resolved node
             to avoid multiple updates of the callee's profile when inlining
             into COMDAT copies.  Both AutoFDO and regular FDO will have the
             same edge counts for all unresolved nodes.  In AutoFDO this is due
             to profile correlation by function name and source position.  In
             regular FDO this is due to COMDAT profile merging performed on the
             dynamic callgraph at the end of LIPO profiling.  */
	  if (L_IPO_COMP_MODE && cgraph_pre_profiling_inlining_done)
	    {
	      struct cgraph_node *caller = e->caller;
	      if (caller->global.inlined_to)
		caller = caller->global.inlined_to;
	      if (cgraph_lipo_get_resolved_node (caller->decl) != caller)
		update_original = false;
	    }
	  if (freq_scale == -1)
	    freq_scale = e->frequency;
	  n = cgraph_clone_node (e->callee, e->callee->decl,
				 MIN(e->count, e->callee->count), freq_scale,
				 update_original, vNULL, true, inlining_into,
				 NULL);
	  n->used_as_abstract_origin = e->callee->used_as_abstract_origin;
	  cgraph_redirect_edge_callee (e, n);
	}
    }
  else
    symtab_dissolve_same_comdat_group_list (e->callee);

  e->callee->global.inlined_to = inlining_into;

  /* Recursively clone all bodies.  */
  for (e = e->callee->callees; e; e = next)
    {
      next = e->next_callee;
      if (!e->inline_failed)
        clone_inlined_nodes (e, duplicate, update_original, overall_size, freq_scale);
      if (e->speculative && !speculation_useful_p (e, true))
	{
	  cgraph_resolve_speculation (e, NULL);
	  speculation_removed = true;
	}
    }
}

#define MAX_INT_LENGTH 16

/* Return NODE's name and aux info. The output is controled by OPT_INFO
   level.  */

static const char *
cgraph_node_opt_info (struct cgraph_node *node, bool emit_mod_info)
{
  char *buf;
  size_t buf_size;
  const char *bfd_name = lang_hooks.dwarf_name (node->decl, 0);
  const char *mod_name = 0;
  unsigned int mod_id = 0;
  int funcdef_no = -1;
  const char *primary_tag = 0;

  if (!bfd_name)
    bfd_name = "unknown";

  buf_size = strlen (bfd_name) + 1;
  if (profile_info)
    buf_size += (MAX_INT_LENGTH + 3);

  if (L_IPO_COMP_MODE && emit_mod_info)
    {
      mod_id = cgraph_get_module_id (node->decl);
      gcc_assert (mod_id);
      mod_name = get_module_name (mod_id);
      primary_tag = (mod_id == primary_module_id ? "*" :"");
      buf_size += (4 + strlen (mod_name));
      if (PARAM_VALUE (PARAM_INLINE_DUMP_MODULE_ID))
        {
          struct function *func = DECL_STRUCT_FUNCTION (node->decl);
          if (func)
            funcdef_no = func->funcdef_no; 
          buf_size += (2 * MAX_INT_LENGTH + 1);
	}
    }

  buf = (char *) xmalloc (buf_size);

  strcpy (buf, bfd_name);

  if (L_IPO_COMP_MODE && emit_mod_info)
    {
      if (PARAM_VALUE (PARAM_INLINE_DUMP_MODULE_ID))
         sprintf (buf, "%s [%d:%d %s%s]", buf, mod_id, funcdef_no,
	     primary_tag, mod_name);
      else
         sprintf (buf, "%s [%s%s]", buf, primary_tag, mod_name);
    }

  if (profile_info)
    sprintf (buf, "%s ("HOST_WIDEST_INT_PRINT_DEC")", buf, node->count);
  return buf;
}

/* Return CALLER's inlined call chain. Save the cgraph_node of the ultimate
   function that the caller is inlined to in FINAL_CALLER.  */

static const char *
cgraph_node_call_chain (struct cgraph_node *caller,
		        struct cgraph_node **final_caller)
{
  struct cgraph_node *node;
  const char *via_str = " (via inline instance";
  size_t current_string_len = strlen (via_str) + 1;
  size_t buf_size = current_string_len;
  char *buf = (char *) xmalloc (buf_size);

  buf[0] = 0;
  gcc_assert (caller->global.inlined_to != NULL);
  strcat (buf, via_str);
  for (node = caller; node->global.inlined_to != NULL;
       node = node->callers->caller)
    {
      const char *name = cgraph_node_opt_info (node, false);
      current_string_len += (strlen (name) + 1);
      if (current_string_len >= buf_size)
	{
	  buf_size = current_string_len * 2;
	  buf = (char *) xrealloc (buf, buf_size);
	}
      strcat (buf, " ");
      strcat (buf, name);
    }
  strcat (buf, ")");
  *final_caller = node;
  return buf;
}

/* Dump the inline decision of EDGE to stderr.  */

static void
dump_inline_decision (struct cgraph_edge *edge)
{
  location_t locus;
  const char *inline_chain_text;
  const char *call_count_text;
  struct cgraph_node *final_caller = edge->caller;

  if (final_caller->global.inlined_to != NULL)
    inline_chain_text = cgraph_node_call_chain (final_caller, &final_caller);
  else
    inline_chain_text = "";

  if (edge->count > 0)
    {
      const char *call_count_str = " with call count ";
      char *buf = (char *) xmalloc (strlen (call_count_str) + MAX_INT_LENGTH);
      sprintf (buf, "%s"HOST_WIDEST_INT_PRINT_DEC, call_count_str,
	       edge->count);
      call_count_text = buf;
    }
  else
    {
      call_count_text = "";
    }
 
  locus = gimple_location (edge->call_stmt);
  dump_printf_loc (is_in_ipa_inline ? MSG_OPTIMIZED_LOCATIONS : MSG_NOTE,
                   locus,
                   "%s inlined into %s%s%s\n",
                   cgraph_node_opt_info (edge->callee, true),
                   cgraph_node_opt_info (final_caller, true),
                   call_count_text,
                   inline_chain_text);
}

/* Mark edge E as inlined and update callgraph accordingly.  UPDATE_ORIGINAL
   specify whether profile of original function should be updated.  If any new
   indirect edges are discovered in the process, add them to NEW_EDGES, unless
   it is NULL. If UPDATE_OVERALL_SUMMARY is false, do not bother to recompute overall
   size of caller after inlining. Caller is required to eventually do it via
   inline_update_overall_summary.
   If callee_removed is non-NULL, set it to true if we removed callee node.

   Return true iff any new callgraph edges were discovered as a
   result of inlining.  */

bool
inline_call (struct cgraph_edge *e, bool update_original,
	     vec<cgraph_edge_p> *new_edges,
	     int *overall_size, bool update_overall_summary,
	     bool *callee_removed)
{
  int old_size = 0, new_size = 0;
  struct cgraph_node *to = NULL;
  struct cgraph_edge *curr = e;
  struct cgraph_node *callee = cgraph_function_or_thunk_node (e->callee, NULL);
  bool new_edges_found = false;

  /* Skip fake edge.  */
  if (L_IPO_COMP_MODE && !e->call_stmt)
    return false;

#ifdef ENABLE_CHECKING
  int estimated_growth = estimate_edge_growth (e);
  bool predicated = inline_edge_summary (e)->predicate != NULL;
#endif

  if (dump_enabled_p ())
    dump_inline_decision (e);

  speculation_removed = false;
  /* Don't inline inlined edges.  */
  gcc_assert (e->inline_failed);
  /* Don't even think of inlining inline clone.  */
  gcc_assert (!callee->global.inlined_to);

  e->inline_failed = CIF_OK;
  DECL_POSSIBLY_INLINED (callee->decl) = true;

  to = e->caller;
  if (to->global.inlined_to)
    to = to->global.inlined_to;

  /* If aliases are involved, redirect edge to the actual destination and
     possibly remove the aliases.  */
  if (e->callee != callee)
    {
      struct cgraph_node *alias = e->callee, *next_alias;

      cgraph_redirect_edge_callee (e, callee);
      while (alias && alias != callee)
	{
	  if (!alias->callers
	      && can_remove_node_now_p (alias, e))
	    {
	      next_alias = cgraph_alias_target (alias);
	      cgraph_remove_node (alias);
	      if (callee_removed)
		*callee_removed = true;
	      alias = next_alias;
	    }
	  else
	    break;
	}
    }

  clone_inlined_nodes (e, true, update_original, overall_size, e->frequency);

  gcc_assert (curr->callee->global.inlined_to == to);

  old_size = inline_summary (to)->size;
  inline_merge_summary (e);
  if (optimize)
    new_edges_found = ipa_propagate_indirect_call_infos (curr, new_edges);
  if (update_overall_summary)
   inline_update_overall_summary (to);
  new_size = inline_summary (to)->size;
  if (to->max_bb_count < e->callee->max_bb_count)
    to->max_bb_count = e->callee->max_bb_count;

  if (callee->calls_comdat_local)
    to->calls_comdat_local = true;
  else if (to->calls_comdat_local && symtab_comdat_local_p (callee))
    {
      struct cgraph_edge *se = to->callees;
      for (; se; se = se->next_callee)
	if (se->inline_failed && symtab_comdat_local_p (se->callee))
	  break;
      if (se == NULL)
	to->calls_comdat_local = false;
    }

#ifdef ENABLE_CHECKING
  /* Verify that estimated growth match real growth.  Allow off-by-one
     error due to INLINE_SIZE_SCALE roudoff errors.  */
  gcc_assert (!update_overall_summary || !overall_size || new_edges_found
	      || abs (estimated_growth - (new_size - old_size)) <= 1
	      || speculation_removed
	      /* FIXME: a hack.  Edges with false predicate are accounted
		 wrong, we should remove them from callgraph.  */
	      || predicated
              /* FIXME_LIPO -- LIPO is not yet compatible
                 with ipa devirt. */
              || flag_dyn_ipa);
#endif

  /* Account the change of overall unit size; external functions will be
     removed and are thus not accounted.  */
  if (overall_size
      && !DECL_EXTERNAL (to->decl)
      && ((L_IPO_COMP_MODE
           && cgraph_get_module_id (to->decl)
           == primary_module_id)
          || !L_IPO_COMP_MODE))
    *overall_size += new_size - old_size;
  ncalls_inlined++;

  /* This must happen after inline_merge_summary that rely on jump
     functions of callee to not be updated.  */
  return new_edges_found;
}


/* Copy function body of NODE and redirect all inline clones to it.
   This is done before inline plan is applied to NODE when there are
   still some inline clones if it.

   This is necessary because inline decisions are not really transitive
   and the other inline clones may have different bodies.  */

static struct cgraph_node *
save_inline_function_body (struct cgraph_node *node)
{
  struct cgraph_node *first_clone, *n;

  if (dump_file)
    fprintf (dump_file, "\nSaving body of %s for later reuse\n",
	     node->name ());
 
  gcc_assert (node == cgraph_get_node (node->decl));

  /* first_clone will be turned into real function.  */
  first_clone = node->clones;
  first_clone->decl = copy_node (node->decl);
  symtab_insert_node_to_hashtable (first_clone);
  gcc_assert (first_clone == cgraph_get_node (first_clone->decl));

  /* Now reshape the clone tree, so all other clones descends from
     first_clone.  */
  if (first_clone->next_sibling_clone)
    {
      for (n = first_clone->next_sibling_clone; n->next_sibling_clone; n = n->next_sibling_clone)
        n->clone_of = first_clone;
      n->clone_of = first_clone;
      n->next_sibling_clone = first_clone->clones;
      if (first_clone->clones)
        first_clone->clones->prev_sibling_clone = n;
      first_clone->clones = first_clone->next_sibling_clone;
      first_clone->next_sibling_clone->prev_sibling_clone = NULL;
      first_clone->next_sibling_clone = NULL;
      gcc_assert (!first_clone->prev_sibling_clone);
    }
  first_clone->clone_of = NULL;

  /* Now node in question has no clones.  */
  node->clones = NULL;

  /* Inline clones share decl with the function they are cloned
     from.  Walk the whole clone tree and redirect them all to the
     new decl.  */
  if (first_clone->clones)
    for (n = first_clone->clones; n != first_clone;)
      {
        gcc_assert (n->decl == node->decl);
	n->decl = first_clone->decl;
	if (n->clones)
	  n = n->clones;
	else if (n->next_sibling_clone)
	  n = n->next_sibling_clone;
	else
	  {
	    while (n != first_clone && !n->next_sibling_clone)
	      n = n->clone_of;
	    if (n != first_clone)
	      n = n->next_sibling_clone;
	  }
      }

  /* Copy the OLD_VERSION_NODE function tree to the new version.  */
  tree_function_versioning (node->decl, first_clone->decl,
			    NULL, true, NULL, false,
			    NULL, NULL);

  /* The function will be short lived and removed after we inline all the clones,
     but make it internal so we won't confuse ourself.  */
  DECL_EXTERNAL (first_clone->decl) = 0;
  TREE_PUBLIC (first_clone->decl) = 0;
  DECL_COMDAT (first_clone->decl) = 0;
  first_clone->ipa_transforms_to_apply.release ();

  /* When doing recursive inlining, the clone may become unnecessary.
     This is possible i.e. in the case when the recursive function is proved to be
     non-throwing and the recursion happens only in the EH landing pad.
     We can not remove the clone until we are done with saving the body.
     Remove it now.  */
  if (!first_clone->callers)
    {
      cgraph_remove_node_and_inline_clones (first_clone, NULL);
      first_clone = NULL;
    }
#ifdef ENABLE_CHECKING
  else
    verify_cgraph_node (first_clone);
#endif
  return first_clone;
}

/* Return true when function body of DECL still needs to be kept around
   for later re-use.  */
static bool
preserve_function_body_p (struct cgraph_node *node)
{
  gcc_assert (cgraph_global_info_ready);
  gcc_assert (!node->alias && !node->thunk.thunk_p);

  /* Look if there is any clone around.  */
  if (node->clones)
    return true;
  return false;
}

/* Apply inline plan to function.  */

unsigned int
inline_transform (struct cgraph_node *node)
{
  unsigned int todo = 0;
  struct cgraph_edge *e, *next;
 
  /* FIXME: Currently the pass manager is adding inline transform more than
     once to some clones.  This needs revisiting after WPA cleanups.  */
  if (cfun->after_inlining)
    return 0;

  /* We might need the body of this function so that we can expand
     it inline somewhere else.  */
  if (preserve_function_body_p (node))
    save_inline_function_body (node);

  for (e = node->callees; e; e = next)
    {
      next = e->next_callee;
      cgraph_redirect_edge_call_stmt_to_callee (e);
    }
  ipa_remove_all_references (&node->ref_list);

  timevar_push (TV_INTEGRATION);
  if (node->callees && optimize)
    todo = optimize_inline_calls (current_function_decl);
  timevar_pop (TV_INTEGRATION);

  cfun->always_inline_functions_inlined = true;
  cfun->after_inlining = true;
  todo |= execute_fixup_cfg ();

  if (!(todo & TODO_update_ssa_any))
    /* Redirecting edges might lead to a need for vops to be recomputed.  */
    todo |= TODO_update_ssa_only_virtuals;

  return todo;
}
