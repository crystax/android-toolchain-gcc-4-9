/* ServiceUIFactory.java --
   Copyright (C) 2004, 2006 Free Software Foundation, Inc.

This file is part of GNU Classpath.

GNU Classpath is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Classpath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */


package javax.print;

/**
 * <code>ServiceUIFactory</code> enables print services to provide additional
 * user interface dialogs.
 * <p>
 * A print service may provide a <code>ServiceUIFactory</code> implementation
 * if its <code>getServiceUIFactory()</code> method is called. If a factory
 * object is returned it can be queried for provided user interface dialogs.
 * Different roles are defined to denote dialogs providing informations about
 * the print service, dialogs for administration of a print service and for
 * end-user browsing dialogs.
 * </p><p>
 * The factory can support providing these UI roles in different dialog types
 * (AWT, Swing, JComponent, Panel). The support and use of Swing interfaces is
 * however preferred.
 * </p>
 *
 * @author Michael Koch
 */
public abstract class ServiceUIFactory
{
  /** A user interface providing informations about the print service. */
  public static final int ABOUT_UIROLE = 1;

  /** A user interface to administer the print service. */
  public static final int ADMIN_UIROLE = 2;

  /** A user interface for end-user browsing of the print service. */
  public static final int MAIN_UIROLE = 3;

  /** Role IDs greater than this may be used for other private roles. */
  public static final int RESERVED_UIROLE = 99;

  /** Identifies a UI provided as an AWT dialog. */
  public static final String DIALOG_UI = "java.awt.Dialog";

  /** Identifies a UI provided as a Swing JComponent. */
  public static final String JCOMPONENT_UI = "javax.swing.JComponent";

  /** Identifies a UI provided as a Swing JDialog. */
  public static final String JDIALOG_UI = "javax.swing.JDialog";

  /** Identifies a UI provided as an AWT Panel. */
  public static final String PANEL_UI = "java.awt.Panel";

  /**
   * Constructs a <code>ServiceUIFactory</code> object.
   */
  public ServiceUIFactory()
  {
        // Do nothing here.
  }

  /**
   * Returns an UI object which may be cast to the requested UI type.
   *
   * @param role the role requested. Must be one of the standard roles
   * or a private role supported by this factory
   * @param ui type in which the role is requested
   *
   * @return the UI role or null of this role is not supported by this factory
   *
   * @throws IllegalArgumentException if <code>role</code> is neither one of
   * the standard ones nor a private one supported by this factory
   */
  public abstract Object getUI(int role, String ui);

  /**
   * Returns the UI types supported by this factory for an UI role.
   *
   * @param role the role to be looked up
   *
   * @return an array of UI types
   *
   * @throws IllegalArgumentException if <code>role</code> is neither one of
   * the standard ones nor a private one supported by this factory
   */
  public abstract String[] getUIClassNamesForRole(int role);
}
