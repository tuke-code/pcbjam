/*
 * test_kimath.cpp - Test kimath and sexpr without wxWidgets
 */

#include <iostream>
#include <math/vector2d.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <sexpr/sexpr.h>
#include <sexpr/sexpr_parser.h>

int main() {
    std::cout << "Testing kimath standalone build...\n";

    // Test VECTOR2I
    VECTOR2I p1(0, 0);
    VECTOR2I p2(100, 100);
    std::cout << "Created VECTOR2I: (" << p1.x << "," << p1.y << ") and ("
              << p2.x << "," << p2.y << ")\n";

    // Test SEG
    SEG segment(p1, p2);
    int length = segment.Length();
    std::cout << "SEG length: " << length << "\n";

    // Test SHAPE_POLY_SET
    SHAPE_POLY_SET poly;
    poly.NewOutline();
    poly.Append(0, 0);
    poly.Append(1000, 0);
    poly.Append(1000, 1000);
    poly.Append(0, 1000);

    std::cout << "Created polygon with " << poly.OutlineCount() << " outline(s)\n";
    std::cout << "Polygon area: " << poly.Area() << "\n";

    // Test S-expression parser
    std::cout << "\nTesting S-expression parser...\n";
    std::string test_sexpr = "(kicad_pcb (version 20231014) (generator \"test\") (layer F.Cu front copper))";

    SEXPR::PARSER parser;
    std::unique_ptr<SEXPR::SEXPR> parsed = parser.Parse(test_sexpr);

    if (parsed && parsed->IsList()) {
        const SEXPR::SEXPR_VECTOR* list = parsed->GetChildren();
        std::cout << "Parsed S-expr with " << list->size() << " elements\n";
        if (!list->empty() && (*list)[0]->IsSymbol()) {
            std::cout << "Root element: " << (*list)[0]->GetSymbol() << "\n";
        }
    } else {
        std::cout << "Failed to parse S-expression\n";
        return 1;
    }

    std::cout << "\nkimath + sexpr standalone build: SUCCESS!\n";
    return 0;
}
