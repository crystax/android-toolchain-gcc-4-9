// PR c++/43452

class Foo;         // { dg-warning "forward" }
int main() {
   Foo* p;         // { dg-warning "incomplete" }
   delete [] p;    // { dg-warning "problem" }
}
