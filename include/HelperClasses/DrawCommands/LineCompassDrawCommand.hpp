#pragma once

#include "TextDrawCommand.hpp"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // LineCompassDrawCommand
    // -------------------------------------------------------------------------
    // Draws a compass with a line pointing in the direction of the current GPS heading. 

    class LineCompassDrawCommand : public TextDrawCommand
    {   
    public:
        static constexpr size_t CHAR_WIDTH = 6;
        static constexpr size_t DISTANCE_TEXT_WIDTH = 6;

        LineCompassDrawCommand(int heading, int distanceMeters, uint8_t textLine) : _heading(heading), _distanceMeters(distanceMeters)
        {
            format = TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, textLine };
        }

        void draw(DrawContext &ctx) override
        {
            // Format distance text
            if (_distanceMeters >= 1000)
            {
                text = std::to_string(_distanceMeters / 1000) + "km";
            }
            else
            {
                text = std::to_string(_distanceMeters) + "m";
            }
    
            // Pad or truncate to fixed width so centering is stable
            if (text.length() < DISTANCE_TEXT_WIDTH)
            {
                size_t pad = DISTANCE_TEXT_WIDTH - text.length();
                size_t padLeft = pad / 2;
                size_t padRight = pad - padLeft;
                text = std::string(padLeft, ' ') + text + std::string(padRight, ' ');
            }
            else if (text.length() > DISTANCE_TEXT_WIDTH)
            {
                text = text.substr(0, DISTANCE_TEXT_WIDTH);
            }
    
            // Layout calculation — scales with screen width
            size_t totalChars = ctx.width / CHAR_WIDTH;
            size_t sideSpace  = (totalChars - DISTANCE_TEXT_WIDTH) / 2;
            size_t arrowSpace = sideSpace - 1; // reserve 1 char gap between text and arrow
    
            // States: front + right growth(arrowSpace) + back + left shrink(arrowSpace)
            size_t animationStates = arrowSpace * 2 + 2;
    
            // Degrees per state, offset so 0° sits in the center of state 0
            double stateSizeDegrees = 360.0 / animationStates;
            int state = static_cast<int>(round((_heading + (stateSizeDegrees / 2.0)) / stateSizeDegrees)) % animationStates;
    
            // States (example for 128px wide / 6px chars = 21 chars, sideSpace=7, arrowSpace=6):
            //  0 |     >[1000km]<     |  Directly ahead
            //  1 |       1000km >     |  Right arrow grows...
            //  2 |       1000km =>    |
            //  3 |       1000km ==>   |
            //  4 |       1000km ===>  |
            //  5 |       1000km ====> |
            //  6 |       1000km =====>|
            //  7 |<===== 1000km =====>|  Directly behind
            //  8 |<===== 1000km       |  Left arrow shrinks...
            //  9 | <==== 1000km       |
            // 10 |  <=== 1000km       |
            // 11 |   <== 1000km       |
            // 12 |    <= 1000km       |
            // 13 |     < 1000km       |
    
            std::string leftSide;
            std::string rightSide;
    
            if (state == 0)
            {
                // Target directly ahead
                leftSide  = std::string(arrowSpace - 1, ' ') + ">[";
                rightSide = "]<" + std::string(arrowSpace - 1, ' ');
            }
            else if (state <= static_cast<int>(arrowSpace))
            {
                // Target to the right — arrow grows from text outward
                size_t eqCount   = state - 1;
                size_t trailPad  = arrowSpace - eqCount - 1;
                leftSide  = std::string(sideSpace, ' ');
                rightSide = " " + std::string(eqCount, '=') + ">" + std::string(trailPad, ' ');
            }
            else if (state == static_cast<int>(arrowSpace) + 1)
            {
                // Target directly behind
                leftSide  = "<" + std::string(arrowSpace - 1, '=') + " ";
                rightSide = " " + std::string(arrowSpace - 1, '=') + ">";
            }
            else
            {
                // Target to the left — arrow shrinks toward text
                size_t stepsFromBack = state - (arrowSpace + 1);
                size_t eqCount  = arrowSpace - stepsFromBack;
                size_t leadPad  = sideSpace - eqCount - 1 - 1;
                leftSide  = std::string(leadPad, ' ') + "<" + std::string(eqCount, '=') + " ";
                rightSide = std::string(sideSpace, ' ');
            }
    
            text = leftSide + text + rightSide;
    
            TextDrawCommand::draw(ctx);
        }

    private:
        int _heading;
        int _distanceMeters;
    };
}