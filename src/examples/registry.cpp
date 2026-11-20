#include "example.h"

std::unique_ptr<Example> makeSoftShadows();
std::unique_ptr<Example> makeColoredLights();
std::unique_ptr<Example> makeBumpMapping();
std::unique_ptr<Example> makeFlickerBlink();
std::unique_ptr<Example> makeLaserMirrors();
std::unique_ptr<Example> makeThunderstorm();
std::unique_ptr<Example> makeTeslaCoil();
std::unique_ptr<Example> makeSearchlights();
std::unique_ptr<Example> makeSandbox();
std::unique_ptr<Example> makeClassic();

const std::vector<ExampleInfo>& exampleList() {
    static const std::vector<ExampleInfo> list = {
        {"Soft shadows", "Area light with a real penumbra: wide far from the occluder, sharp at contact.",
         "Mouse: move the lamp", makeSoftShadows},
        {"Coloured lights", "Orbiting RGB lights mix additively and tint each other's shadows.",
         "Panel: number of lights, orbit speed", makeColoredLights},
        {"Bump mapping", "Normal-mapped bricks, tiles, metal and cobblestone lit per pixel with specular.",
         "Mouse: move the light; panel: surface, height", makeBumpMapping},
        {"Flicker & blink", "Dungeon with candle torches, a blinking beacon, a broken neon tube and a pulsing crystal.",
         "Panel: flicker speed, strobe", makeFlickerBlink},
        {"Laser & mirrors", "HDR laser beams bouncing off rotating mirrors, lighting the floor and throwing sparks.",
         "Mouse: aim the red laser", makeLaserMirrors},
        {"Thunderstorm", "Lightning strikes flash the whole scene with long shadows, rain and a lantern.",
         "Mouse: carry the lantern; click: strike", makeThunderstorm},
        {"Tesla coil", "Crackling electric arcs jump to grounding pylons and to your cursor.",
         "Mouse: attract arcs", makeTeslaCoil},
        {"Searchlights", "Sweeping spotlight cones over a prison yard. Don't get spotted.",
         "Mouse: sneak around", makeSearchlights},
        {"Sandbox", "Place lights and blocks, mix flicker modes and spotlights.",
         "Left click: add/drag light; right click: add/remove block", makeSandbox},
        {"Original (2011)", "The original school project: silhouette points and shadow rays as a debug view.",
         "Hold left button: drag the lamp", makeClassic},
    };
    return list;
}
