#pragma once

#include <functional>
#include <memory>

class Razors;
using RazorsResource = std::unique_ptr<Razors, std::function<void(Razors*)>>;

RazorsResource makeRazors();
void draw(Razors& razors, double ms);
