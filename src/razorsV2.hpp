#pragma once

#include <functional>
#include <memory>

class RazorsV2;
using RazorsV2Resource =
        std::unique_ptr<RazorsV2, std::function<void(RazorsV2*)>>;

RazorsV2Resource makeRazorsV2();
void draw(RazorsV2& razors, double ms);
