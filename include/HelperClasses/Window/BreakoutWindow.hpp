#pragma once

#include <algorithm>
#include <Arduino.h> // millis()
#include "Window.hpp"
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "HelperClasses/DrawCommands/FnDrawCommand.hpp"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // BreakoutGameState
    // -------------------------------------------------------------------------
    // Full breakout game in a single state.
    //
    // Controls:
    //   ENC_UP   → move paddle right
    //   ENC_DOWN → move paddle left
    //   BUTTON_3 → exit (handled by BreakoutWindow)
    //   BUTTON_4 → restart
    //
    // Layout (128 × 128):
    //   Row 0–9    : status bar (score left, lives right)
    //   Row 10–127 : game field (bricks, ball, paddle)

    class BreakoutGameState : public WindowState
    {
    public:
        static constexpr int W           = 128;
        static constexpr int H           = 128;
        static constexpr int STATUS_H    = 10;

        static constexpr int BRICK_COLS  = 6;
        static constexpr int BRICK_ROWS  = 5;
        static constexpr int BRICK_W     = 18;
        static constexpr int BRICK_H     = 5;
        static constexpr int BRICK_GAP_X = 2;
        static constexpr int BRICK_GAP_Y = 2;
        static constexpr int BRICK_LEFT  = (W - (BRICK_COLS * BRICK_W + (BRICK_COLS - 1) * BRICK_GAP_X)) / 2;
        static constexpr int BRICK_TOP   = STATUS_H + 3;

        static constexpr int PADDLE_W    = 24;
        static constexpr int PADDLE_H    = 3;
        static constexpr int PADDLE_Y    = H - 10;
        static constexpr int PADDLE_STEP = 8;

        static constexpr int BALL_SZ     = 3;

        static constexpr int TOTAL_BRICKS = BRICK_ROWS * BRICK_COLS;

        // Render cadence: how often the frame is redrawn (40 FPS).
        static constexpr uint32_t FRAME_MS = 25;
        // Physics cadence: how often the simulation advances one step. The ball
        // velocities are tuned per-step, so this fixes the ball's real speed
        // independent of FRAME_MS.
        static constexpr uint32_t STEP_MS  = 25;
        // Cap on catch-up steps in a single update so a long stall can't make
        // the simulation spiral trying to replay a huge backlog.
        static constexpr int MAX_STEPS = 4;

        BreakoutGameState()
        {
            refreshIntervalMs = FRAME_MS;

            // Bindings live in the constructor so they are registered exactly once.
            // Each paddle move also advances the simulation: a stream of encoder
            // inputs keeps the queue busy so the autonomous refresh timeout (which
            // drives onTick) never fires — without this the ball would freeze for
            // as long as the paddle is moving.
            bindInput(InputID::ENC_UP, [this](const InputContext &)
            {
                _paddleX = std::min(_paddleX + PADDLE_STEP, W - PADDLE_W);
                _stepPhysics();
            });

            bindInput(InputID::ENC_DOWN, [this](const InputContext &)
            {
                _paddleX = std::max(_paddleX - PADDLE_STEP, 0);
                _stepPhysics();
            });

            bindInput(InputID::BUTTON_4, "Restart", [this](const InputContext &)
            {
                _resetGame();
            });
        }

        void onEnter(const StateTransferData &) override
        {
            _resetGame();
            addDrawCommand(std::make_shared<FnDrawCommand>([this](DrawContext &ctx)
            {
                _draw(ctx);
            }));
        }

        void onTick() override
        {
            _stepPhysics();
        }

    private:
        int      _paddleX    = (W - PADDLE_W) / 2;
        float    _ballX      = W / 2.0f;
        float    _ballY      = PADDLE_Y - BALL_SZ - 2.0f;
        float    _velX       = 1.0f;
        float    _velY       = -1.25f;
        int      _score      = 0;
        int      _lives      = 3;
        bool     _over       = false;
        bool     _won        = false;
        bool     _bricks[BRICK_ROWS][BRICK_COLS];
        int      _bricksLeft = TOTAL_BRICKS;
        uint32_t _lastStepMs = 0; // wall clock of the last advanced physics step

        void _resetGame()
        {
            _paddleX    = (W - PADDLE_W) / 2;
            _ballX      = W / 2.0f;
            _ballY      = PADDLE_Y - BALL_SZ - 2.0f;
            _velX       = 1.0f;
            _velY       = -1.25f;
            _score      = 0;
            _lives      = 3;
            _over       = false;
            _won        = false;
            _bricksLeft = TOTAL_BRICKS;
            _lastStepMs = millis();
            for (int r = 0; r < BRICK_ROWS; ++r)
                for (int c = 0; c < BRICK_COLS; ++c)
                    _bricks[r][c] = true;
        }

        static float _fabs(float v) { return v < 0 ? -v : v; }

        // Advance the simulation on a fixed STEP_MS timestep using real elapsed
        // time. Called both from onTick() (autonomous refresh) and from the
        // paddle input handlers, so the ball keeps moving at a constant speed
        // regardless of the render rate or how fast inputs are arriving.
        void _stepPhysics()
        {
            uint32_t now = millis();

            if (_over || _won)
            {
                _lastStepMs = now; // no backlog to replay once play resumes
                return;
            }

            int steps = 0;
            while (now - _lastStepMs >= STEP_MS && steps < MAX_STEPS)
            {
                _update();
                _lastStepMs += STEP_MS;
                ++steps;
                if (_over || _won) break;
            }

            // Dropped frames (long stall): discard the backlog instead of
            // sprinting to catch up, which would teleport the ball.
            if (steps >= MAX_STEPS)
                _lastStepMs = now;
        }

        void _update()
        {
            _ballX += _velX;
            _ballY += _velY;

            // Left / right walls
            if (_ballX < 0)           { _ballX = 0;              _velX =  _fabs(_velX); }
            if (_ballX + BALL_SZ > W) { _ballX = W - BALL_SZ;   _velX = -_fabs(_velX); }

            // Top wall (below status bar)
            if (_ballY < STATUS_H)    { _ballY = STATUS_H;       _velY =  _fabs(_velY); }

            // Paddle collision (only when moving downward)
            if (_velY > 0 &&
                _ballY + BALL_SZ >= PADDLE_Y &&
                _ballY + BALL_SZ <= PADDLE_Y + PADDLE_H + 3 &&
                _ballX + BALL_SZ >  _paddleX &&
                _ballX            <  _paddleX + PADDLE_W)
            {
                _ballY = PADDLE_Y - BALL_SZ;
                _velY  = -_fabs(_velY);
                // Vary X angle based on where ball hits paddle
                float rel = (_ballX + BALL_SZ * 0.5f) - (_paddleX + PADDLE_W * 0.5f);
                _velX = rel * 0.09f;
                if (_fabs(_velX) < 0.3f) _velX = (_velX >= 0) ? 0.3f : -0.3f;
            }

            // Ball escapes below paddle
            if (_ballY > H)
            {
                _lives--;
                if (_lives <= 0)
                {
                    _over = true;
                    return;
                }
                // Reset ball above paddle
                _ballX = _paddleX + PADDLE_W * 0.5f - BALL_SZ * 0.5f;
                _ballY = PADDLE_Y - BALL_SZ - 2.0f;
                _velX  = 1.0f;
                _velY  = -1.25f;
                return;
            }

            // Brick collisions — one brick per tick
            int bx = (int)_ballX;
            int by = (int)_ballY;
            for (int r = 0; r < BRICK_ROWS; ++r)
            {
                for (int c = 0; c < BRICK_COLS; ++c)
                {
                    if (!_bricks[r][c]) continue;

                    int rx = BRICK_LEFT + c * (BRICK_W + BRICK_GAP_X);
                    int ry = BRICK_TOP  + r * (BRICK_H + BRICK_GAP_Y);

                    if (bx + BALL_SZ <= rx || bx >= rx + BRICK_W ||
                        by + BALL_SZ <= ry || by >= ry + BRICK_H)
                        continue;

                    _bricks[r][c] = false;
                    _score += 10;
                    _bricksLeft--;

                    // Reflect off the nearest face
                    float ol = (bx + BALL_SZ) - rx;
                    float or_ = (rx + BRICK_W) - bx;
                    float ot = (by + BALL_SZ) - ry;
                    float ob = (ry + BRICK_H) - by;

                    float minH = ol < or_ ? ol : or_;
                    float minV = ot < ob  ? ot : ob;
                    if (minH < minV) _velX = -_velX;
                    else             _velY = -_velY;

                    if (_bricksLeft == 0) { _won = true; return; }
                    return;
                }
            }
        }

        void _draw(DrawContext &ctx)
        {
            auto *d = ctx.display;
            d->fillScreen(BLACK);

            // --- Status bar ---
            d->setTextSize(1);
            d->setTextColor(DrawCommand::DrawColorPrimary());
            d->setCursor(2, 1);
            d->print("SC:");
            d->print(_score);

            // Lives as small filled squares on the right
            for (int i = 0; i < _lives && i < 5; ++i)
                d->fillRect(W - 4 - i * 5, 2, 3, 3, DrawCommand::DrawColorPrimary());

            d->drawFastHLine(0, STATUS_H - 1, W, DrawCommand::DrawColorPrimary());

            // --- Game Over / Win overlay ---
            if (_over)
            {
                d->setCursor(26, 52);
                d->print("GAME  OVER");
                d->setCursor(26, 64);
                d->print("Score: ");
                d->print(_score);
                d->setCursor(14, 76);
                d->print("[4] Restart");
                return;
            }
            if (_won)
            {
                d->setCursor(32, 52);
                d->print("YOU  WIN!");
                d->setCursor(26, 64);
                d->print("Score: ");
                d->print(_score);
                d->setCursor(14, 76);
                d->print("[4] Again");
                return;
            }

            // --- Bricks ---
            for (int r = 0; r < BRICK_ROWS; ++r)
            {
                for (int c = 0; c < BRICK_COLS; ++c)
                {
                    if (!_bricks[r][c]) continue;
                    int bx = BRICK_LEFT + c * (BRICK_W + BRICK_GAP_X);
                    int by = BRICK_TOP  + r * (BRICK_H + BRICK_GAP_Y);
                    d->fillRect(bx, by, BRICK_W, BRICK_H, DrawCommand::DrawColorPrimary());
                }
            }

            // --- Paddle ---
            d->fillRect(_paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, DrawCommand::DrawColorPrimary());

            // --- Ball ---
            d->fillRect((int)_ballX, (int)_ballY, BALL_SZ, BALL_SZ, DrawCommand::DrawColorPrimary());
        }
    };

    // -------------------------------------------------------------------------
    // BreakoutWindow
    // -------------------------------------------------------------------------
    // Thin window wrapper around BreakoutGameState.
    //
    // Usage:
    //   Utilities::pushWindow(std::make_shared<BreakoutWindow>());

    class BreakoutWindow : public Window
    {
    public:
        BreakoutWindow()
        {
            _game = std::make_shared<BreakoutGameState>();

            registerInput(InputID::BUTTON_3, "Back");
            addInputCommand(InputID::BUTTON_3, [](const InputContext &)
            {
                Utilities::popWindow();
            });

            setInitialState(_game);
        }

    private:
        std::shared_ptr<BreakoutGameState> _game;
    };

} // namespace DisplayModule
