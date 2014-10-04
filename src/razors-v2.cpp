#include "estd.hpp"
#include "razorsV2.hpp"

class RazorsV2
{};

RazorsV2Resource makeRazorsV2()
{
        return estd::make_unique<RazorsV2>();
}

void draw(RazorsV2& self, double ms)
{
}
