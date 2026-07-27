#pragma once

#include <Arduino.h>
#include <stdio.h>

// #include "TFT_eSPI.h"

/**
 * @brief Mathimaduino works on a Seeed Wio Terminal and trains kids in maths
 * and spelling. The idea is that for every correct answer, the child gets
 * pocket money.
 */

inline void playTone(int halfPeriodUs, int durationMs)
{
    for (long i = 0; i < durationMs * 1000L; i += halfPeriodUs * 2)
    {
        digitalWrite(WIO_BUZZER, HIGH);
        delayMicroseconds(halfPeriodUs);
        digitalWrite(WIO_BUZZER, LOW);
        delayMicroseconds(halfPeriodUs);
    }
}

inline void playCorrectSound()
{
    playTone(1519, 120);
    playTone(956, 200);
}

inline void playWrongSound()
{
    playTone(2551, 150);
    playTone(3040, 300);
}

struct TftColor
{
    int r{};
    int g{};
    int b{};
};

inline int32_t makeColor(TftColor color)
{
    // Pack into 16-bit RGB565: 5 bits red, 6 bits green, 5 bits blue.
    return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

inline void putLargerFirst(int& a, int& b)
{
    if (a < b)
    {
        int temp = a;
        a        = b;
        b        = temp;
    }
}

inline void adjustOperandsForPerfectDivision(int& a, int& b)
{
    if (a % b == 0)
    {
        return;
    }
    // Adjust b to be a factor of a
    // If we adjust a instead we may end up with a number larger than maxOperand
    // and avoiding that will complicate the logic.
    for (int i = b; i > 0; --i)
    {
        if (a % i == 0)
        {
            b = i;
            return;
        }
    }
}

namespace colors
{
constexpr TftColor Black{0, 0, 0};
constexpr TftColor White{255, 255, 255};
constexpr TftColor Red{255, 0, 0};
constexpr TftColor Green{0, 255, 0};
constexpr TftColor DarkGreen{0, 128, 0};
constexpr TftColor Maroon{128, 0, 0};
constexpr TftColor Purple{128, 0, 128};
constexpr TftColor Yellow{255, 255, 0};
constexpr TftColor Navy{0, 0, 128};
constexpr TftColor Teal{0, 128, 128};
constexpr TftColor Olive{128, 128, 0};
constexpr TftColor Gray{128, 128, 128};
constexpr TftColor Orange{255, 180, 0};
} // namespace colors

struct Point
{
    int x{};
    int y{};
};

struct RectangleDimensions
{
    int x0{};
    int y0{};
    int width{};
    int height{};
    int radius{};
};

struct KeyColors
{
    TftColor label{};
    TftColor unpressed{};
    TftColor pressed{};
    TftColor outline{};
};

enum class NavigationEvent
{
    None,
    Up,
    Down,
    Left,
    Right,
    Press
};

enum class ButtonEvent
{
    None,
    Left,
    Middle,
    Right
};

using KeyListener = void (*)(char);
template<typename... Listeners>
struct KeyboardListeners
{
    constexpr KeyboardListeners(Listeners... listeners)
        : listeners{listeners...}
    {
    }
    static constexpr size_t size = sizeof...(Listeners);
    KeyListener listeners[size];
};

template<typename... Listeners>
constexpr KeyboardListeners<Listeners...>
makeKeyboardListeners(Listeners... listeners)
{
    return KeyboardListeners<Listeners...>{listeners...};
}

template<typename... Labels>
struct KeyLabels
{
    constexpr KeyLabels(Labels... labels)
        : labels{labels...}
    {
    }
    static constexpr size_t size = sizeof...(Labels);
    char labels[size];
};

template<typename... Labels>
constexpr KeyLabels<Labels...> makeKeyLabels(Labels... labels)
{
    return KeyLabels<Labels...>{labels...};
}

template<int Rows,
         int Columns,
         typename TFT_eSPI,
         typename Labels,
         typename KeyListeners>
class TftKeyboard
{
public:
    TftKeyboard(TFT_eSPI& tft,
                KeyColors colors,
                Labels labels,
                KeyListeners listeners)
        : tft_{tft}
        , labelColor_{makeColor(colors.label)}
        , outlineColor_{makeColor(colors.outline)}
        , unpressedColor_{makeColor(colors.unpressed)}
        , pressedColor_{makeColor(colors.pressed)}
        , labels_{labels}
        , listeners_{listeners}
    {
        static_assert(Labels::size == Rows * Columns,
                      "Number of labels must match Rows * Columns");
    }

    void enableKeyboard(bool enable)
    {
        enabled_ = enable;
    }

    void begin(RectangleDimensions rect)
    {
        rect_ = rect;
        enableKeyboard(true);
    }

    void draw()
    {
        tft_.fillRect(
            rect_.x0, rect_.y0, rect_.width, rect_.height, unpressedColor_);
        int keyWidth  = rect_.width / Columns;
        int keyHeight = rect_.height / Rows;
        tft_.setTextSize(1); // Text size multiplier to 1 since we use drawChar
        for (int row = 0; row < Rows; ++row)
        {
            for (int col = 0; col < Columns; ++col)
            {
                int index = row * Columns + col;
                if (index >= Labels::size)
                {
                    break;
                }
                char label = labels_.labels[index];
                int x      = rect_.x0 + col * keyWidth;
                int y      = rect_.y0 + row * keyHeight;
                tft_.drawRoundRect(
                    x, y, keyWidth, keyHeight, rect_.radius, outlineColor_);
                if (row == selectedRow_ && col == selectedCol_)
                {
                    tft_.fillRoundRect(x + 1,
                                       y + 1,
                                       keyWidth - 2,
                                       keyHeight - 2,
                                       rect_.radius,
                                       pressedColor_);
                }
                tft_.setTextColor(labelColor_);
                tft_.drawChar(
                    label, x + keyWidth / 2 - 4, y + keyHeight / 2 - 8, 2);
            }
        }
    }

    void drawSelectedKey(int previousRow, int previousCol)
    {
        // Redraw the previously selected key with unpressed color
        int keyWidth  = rect_.width / Columns;
        int keyHeight = rect_.height / Rows;
        int prevX     = rect_.x0 + previousCol * keyWidth;
        int prevY     = rect_.y0 + previousRow * keyHeight;
        tft_.fillRoundRect(prevX + 1,
                           prevY + 1,
                           keyWidth - 2,
                           keyHeight - 2,
                           rect_.radius,
                           unpressedColor_);
        // Draw the label for the previously selected key
        int prevIndex = previousRow * Columns + previousCol;
        tft_.setTextSize(1); // Text size multiplier to 1 since we use drawChar
        if (prevIndex < Labels::size)
        {
            char prevLabel = labels_.labels[prevIndex];
            tft_.setTextColor(labelColor_);
            tft_.drawChar(prevLabel,
                          prevX + keyWidth / 2 - 4,
                          prevY + keyHeight / 2 - 8,
                          2);
        }
        // Draw the newly selected key with pressed color
        int newX = rect_.x0 + selectedCol_ * keyWidth;
        int newY = rect_.y0 + selectedRow_ * keyHeight;
        tft_.fillRoundRect(newX + 1,
                           newY + 1,
                           keyWidth - 2,
                           keyHeight - 2,
                           rect_.radius,
                           pressedColor_);
        // Draw the label for the newly selected key
        int newIndex = selectedRow_ * Columns + selectedCol_;
        if (newIndex < Labels::size)
        {
            char newLabel = labels_.labels[newIndex];
            tft_.setTextColor(labelColor_);
            tft_.drawChar(
                newLabel, newX + keyWidth / 2 - 4, newY + keyHeight / 2 - 8, 2);
        }
    }

    void handleNavigationEvent(NavigationEvent event)
    {
        if (!enabled_)
        {
            return;
        }
        // Select the appropriate key based on the navigation event
        int previousRow = selectedRow_;
        int previousCol = selectedCol_;
        switch (event)
        {
        case NavigationEvent::Up:
            selectedRow_ = (selectedRow_ - 1 + Rows) % Rows;
            selectedCol_ = (selectedCol_ + Columns) % Columns;
            break;
        case NavigationEvent::Down:
            selectedRow_ = (selectedRow_ + 1) % Rows;
            selectedCol_ = (selectedCol_ + Columns) % Columns;
            break;
        case NavigationEvent::Left:
            selectedRow_ = (selectedRow_ + Rows) % Rows;
            selectedCol_ = (selectedCol_ - 1 + Columns) % Columns;
            break;
        case NavigationEvent::Right:
            selectedRow_ = (selectedRow_ + Rows) % Rows;
            selectedCol_ = (selectedCol_ + 1) % Columns;
            break;
        case NavigationEvent::Press:
        {
            int index = selectedRow_ * Columns + selectedCol_;
            if (index < Labels::size)
            {
                char key = labels_.labels[index];
                for (const auto& listener : listeners_.listeners)
                {
                    listener(key);
                }
            }
        }
        break;
        default:
            break;
        }
        if (previousRow != selectedRow_ || previousCol != selectedCol_)
        {
            drawSelectedKey(previousRow, previousCol);
        }
    }

private:
    TFT_eSPI& tft_;
    int32_t labelColor_;
    int32_t outlineColor_;
    int32_t unpressedColor_;
    int32_t pressedColor_;
    Labels labels_;
    KeyListeners listeners_;

    RectangleDimensions rect_{};
    int selectedRow_{0};
    int selectedCol_{0};
    bool enabled_{false};
};

template<int Rows,
         int Columns,
         typename TFT_eSPI,
         typename Labels,
         typename KeyListeners>
auto makeKeyboard(TFT_eSPI& tft,
                  KeyColors colors,
                  Labels labels,
                  KeyListeners listeners)
{
    return TftKeyboard<Rows, Columns, TFT_eSPI, Labels, KeyListeners>{
        tft, colors, labels, listeners};
}

using QuizListener = void (*)();

template<typename... Listeners>
struct QuizListeners
{
    constexpr QuizListeners(Listeners... listeners)
        : listeners{listeners...}
    {
    }
    static constexpr size_t size = sizeof...(Listeners);
    QuizListener listeners[size];
};

template<typename... Listeners>
constexpr QuizListeners<Listeners...> makeQuizListeners(Listeners... listeners)
{
    return QuizListeners<Listeners...>{listeners...};
}

struct MathsQuestion
{
    int operand1{};
    int operand2{};
    char operation{};
    int answer{}; // OK to assume integer division for now
};

/// The MathsQuiz class is responsible for managing the maths quiz logic, i.e.
/// generating the (current and next) questions and checking the answers
template<typename TFT_eSPI, typename Listeners>
class MathsQuiz
{
public:
    MathsQuiz(TFT_eSPI& tft,
              TftColor backgroundColor,
              TftColor textColor,
              Listeners listeners)
        : tft_{tft}
        , backgroundColor_{makeColor(backgroundColor)}
        , textColor_{makeColor(textColor)}
        , listeners_{listeners}
    {
    }

    void enableQuiz(bool enable)
    {
        enabled_ = enable;
    }

    void begin(RectangleDimensions rect)
    {
        rect_ = rect;
        enableQuiz(true);
    }

    void drawNewQuestion()
    {
        auto currentQuestion  = generateQuestion();
        currentCorrectAnswer_ = currentQuestion.answer;
        tft_.setTextColor(textColor_, backgroundColor_);
        tft_.setTextSize(3);
        tft_.setTextDatum(MC_DATUM);
        tft_.setTextPadding(rect_.width);
        // Format the question as "operand1 operation operand2 = ?"
        snprintf(questionBuffer_,
                 questionBufferSize,
                 "%d %c %d = ",
                 currentQuestion.operand1,
                 currentQuestion.operation,
                 currentQuestion.operand2);
        tft_.drawString(questionBuffer_,
                        rect_.x0 + rect_.width / 2,
                        rect_.y0 + rect_.height / 5);
        userAnswerIndex_ = strlen(questionBuffer_);
    }

    void handleKeyboardPress(char key)
    {
        size_t currentLength = strlen(questionBuffer_);
        if (currentLength < questionBufferSize - 1)
        {
            questionBuffer_[currentLength]     = key;
            questionBuffer_[currentLength + 1] = '\0';
            tft_.setTextColor(textColor_, backgroundColor_);
            tft_.setTextSize(3);
            tft_.setTextDatum(MC_DATUM);
            tft_.setTextPadding(rect_.width);
            tft_.drawString(questionBuffer_,
                            rect_.x0 + rect_.width / 2,
                            rect_.y0 + rect_.height / 5);
        }
    }

    void handleButtonEvent(ButtonEvent event)
    {
        if (!enabled_)
        {
            return;
        }
        switch (event)
        {
        case ButtonEvent::Left:
        {
            // Check the answer
            int userAnswer = atoi(&questionBuffer_[userAnswerIndex_]);
            if (userAnswer == currentCorrectAnswer_)
            {
                // Correct answer, draw a new question
                playCorrectSound();
                for (const auto& listener : listeners_.listeners)
                {
                    listener();
                }
                drawNewQuestion();
            }
            else
            {
                // Incorrect answer, clear the user input
                playWrongSound();
                questionBuffer_[userAnswerIndex_] = '\0';
                tft_.setTextColor(textColor_, backgroundColor_);
                tft_.setTextSize(3);
                tft_.setTextDatum(MC_DATUM);
                tft_.setTextPadding(rect_.width);
                tft_.drawString(questionBuffer_,
                                rect_.x0 + rect_.width / 2,
                                rect_.y0 + rect_.height / 5);
            }
        }
        break;
        case ButtonEvent::Middle:
        {
            // Clear the last character of the user input
            size_t currentLength = strlen(questionBuffer_);
            if (currentLength > userAnswerIndex_)
            {
                questionBuffer_[currentLength - 1] = '\0';
                tft_.setTextColor(textColor_, backgroundColor_);
                tft_.setTextSize(3);
                tft_.setTextDatum(MC_DATUM);
                tft_.setTextPadding(rect_.width);
                tft_.drawString(questionBuffer_,
                                rect_.x0 + rect_.width / 2,
                                rect_.y0 + rect_.height / 5);
            }
        }
        break;
        case ButtonEvent::Right:
        {
            // Clear the entire user input
            questionBuffer_[userAnswerIndex_] = '\0';
            tft_.setTextColor(textColor_, backgroundColor_);
            tft_.setTextSize(3);
            tft_.setTextDatum(MC_DATUM);
            tft_.setTextPadding(rect_.width);
            tft_.drawString(questionBuffer_,
                            rect_.x0 + rect_.width / 2,
                            rect_.y0 + rect_.height / 5);
        }
        break;
        default:
            break;
        }
    }

private:
    TFT_eSPI& tft_;
    int32_t backgroundColor_;
    int32_t textColor_;
    Listeners listeners_;
    RectangleDimensions rect_{};
    int currentCorrectAnswer_{};
    bool enabled_{false};

    constexpr static int minOperand               = 1;
    constexpr static int maxOperand               = 100;
    constexpr static unsigned long operationsSize = 4;
    const char operations[operationsSize]         = {'+', '-', '*', '/'};
    /// Enough to contain something like "nnn + nnn = nnnnn" + null terminator
    constexpr static size_t questionBufferSize = 18;
    char questionBuffer_[questionBufferSize]   = {'\0'};
    size_t userAnswerIndex_{}; // Index of user answer in questionBuffer_

    MathsQuestion generateQuestion()
    {
        int operand1   = random(minOperand, maxOperand + 1);
        int operand2   = random(minOperand, maxOperand + 1);
        char operation = operations[random(0, operationsSize)];
        switch (operation)
        {
        case '+':
            return MathsQuestion{
                operand1, operand2, operation, operand1 + operand2};
        case '-':
            putLargerFirst(operand1, operand2);
            return MathsQuestion{
                operand1, operand2, operation, operand1 - operand2};
        case '*':
            return MathsQuestion{
                operand1, operand2, operation, operand1 * operand2};
        case '/':
            putLargerFirst(operand1, operand2);
            adjustOperandsForPerfectDivision(operand1, operand2);
            return MathsQuestion{
                operand1, operand2, operation, operand1 / operand2};
        default:
            return MathsQuestion{0, 0, '+', 0};
        }
    }
};

template<typename TFT_eSPI, typename Listeners>
MathsQuiz<TFT_eSPI, Listeners> makeMathsQuiz(TFT_eSPI& tft,
                                             TftColor backgroundColor,
                                             TftColor textColor,
                                             Listeners listeners)
{
    return MathsQuiz<TFT_eSPI, Listeners>{
        tft, backgroundColor, textColor, listeners};
}

template<typename TFT_eSPI>
class Label
{
public:
    Label(TFT_eSPI& tft,
          TftColor backgroundColor,
          TftColor textColor,
          int textSize = 1)
        : tft_{tft}
        , backgroundColor_{makeColor(backgroundColor)}
        , textColor_{makeColor(textColor)}
        , textSize_{textSize}
    {
    }

    void begin(Point point)
    {
        point_ = point;
    }

    void draw(const char* label)
    {
        tft_.setTextColor(textColor_, backgroundColor_);
        tft_.setTextSize(textSize_);
        tft_.setTextPadding(tft_.width() / 9);
        tft_.drawString(label, point_.x, point_.y);
    }

private:
    TFT_eSPI& tft_;
    int32_t backgroundColor_;
    int32_t textColor_;
    int textSize_;
    Point point_{};
};

template<typename TFT_eSPI>
Label<TFT_eSPI> makeLabel(TFT_eSPI& tft,
                          TftColor backgroundColor,
                          TftColor textColor,
                          int textSize = 1)
{
    return Label<TFT_eSPI>{tft, backgroundColor, textColor, textSize};
}

template<typename Lbl, typename TFT_eSPI>
class ScoreKeeper
{
public:
    ScoreKeeper(Lbl& label, TFT_eSPI& tft)
        : label_{label}
        , tft_{tft}
    {
    }

    void increment()
    {
        ++score_;
        draw();
    }

    void decrement()
    {
        score_ = 0;
        draw();
    }

    void draw()
    {
        tft_.setTextDatum(TR_DATUM);
        snprintf(scoreBuffer_, sizeof(scoreBuffer_), "# %d", score_);
        label_.draw(scoreBuffer_);
    }

private:
    Lbl& label_;
    TFT_eSPI& tft_;
    int score_{0};
    char scoreBuffer_[16] = {'\0'};
};

template<typename Lbl, typename TFT_eSPI>
ScoreKeeper<Lbl, TFT_eSPI> makeScoreKeeper(Lbl& label, TFT_eSPI& tft)
{
    return ScoreKeeper<Lbl, TFT_eSPI>{label, tft};
}

using ButtonListener = void (*)(ButtonEvent);
template<typename... Listeners>
struct ButtonListeners
{
    constexpr ButtonListeners(Listeners... listeners)
        : listeners{listeners...}
    {
    }
    static constexpr size_t size = sizeof...(Listeners);
    ButtonListener listeners[size];
};

template<typename... Listeners>
constexpr ButtonListeners<Listeners...>
makeButtonListeners(Listeners... listeners)
{
    return ButtonListeners<Listeners...>{listeners...};
}

using NavigationListener = void (*)(NavigationEvent);
template<typename... Listeners>
struct NavigationListeners
{
    constexpr NavigationListeners(Listeners... listeners)
        : listeners{listeners...}
    {
    }
    static constexpr size_t size = sizeof...(Listeners);
    NavigationListener listeners[size];
};

template<typename... Listeners>
constexpr NavigationListeners<Listeners...>
makeNavigationListeners(Listeners... listeners)
{
    return NavigationListeners<Listeners...>{listeners...};
}

template<typename NavigationListeners, typename ButtonListeners>
class InputHandler
{
public:
    InputHandler(NavigationListeners navigationListeners,
                 ButtonListeners buttonListeners)
        : navigationListeners_{navigationListeners}
        , buttonListeners_{buttonListeners}
    {
    }

    /// Called by the navigation button ISRs
    void updateNavigationEvent(NavigationEvent event)
    {
        lastNavigationEvent_     = event;
        lastNavigationEventTime_ = millis();
    }

    /// Called by the push button ISRs
    void updateButtonEvent(ButtonEvent event)
    {
        lastButtonEvent_     = event;
        lastButtonEventTime_ = millis();
    }

    /// Must be called in the main loop to process events
    void handleEvents()
    {
        const auto currentTime = millis();
        if (lastNavigationEvent_ != NavigationEvent::None)
        {
            if (currentTime - lastNavigationEventTime_ > debounceDelay_)
            {
                for (const auto& listener : navigationListeners_.listeners)
                {
                    listener(lastNavigationEvent_);
                }
                lastNavigationEvent_ = NavigationEvent::None;
            }
        }
        if (lastButtonEvent_ != ButtonEvent::None)
        {
            if (currentTime - lastButtonEventTime_ > debounceDelay_)
            {
                for (const auto& listener : buttonListeners_.listeners)
                {
                    listener(lastButtonEvent_);
                }
                lastButtonEvent_ = ButtonEvent::None;
            }
        }
        // Special handling for WIO_KEY_A since we can't use an interrupt for it
        else if (digitalRead(WIO_KEY_A) == LOW)
        {
            // Longer debounce needed for polling at LOW state (opposed to an
            // edge) because the button is held down "longer" than a single edge
            // event.
            if (currentTime - lastButtonEventTime_ > debounceDelay_ * 5)
            {
                lastButtonEventTime_ = currentTime;
                for (const auto& listener : buttonListeners_.listeners)
                {
                    listener(ButtonEvent::Right);
                }
                lastButtonEvent_ = ButtonEvent::None;
            }
        }
    }

private:
    NavigationListeners navigationListeners_;
    ButtonListeners buttonListeners_;
    volatile NavigationEvent lastNavigationEvent_{NavigationEvent::None};
    volatile unsigned long lastNavigationEventTime_{0};
    volatile ButtonEvent lastButtonEvent_{ButtonEvent::None};
    volatile unsigned long lastButtonEventTime_{0};
    constexpr static unsigned long debounceDelay_{50}; // milliseconds
};

template<typename NavigationListeners, typename ButtonListeners>
InputHandler<NavigationListeners, ButtonListeners>
makeInputHandler(NavigationListeners navigationListeners,
                 ButtonListeners buttonListeners)
{
    return InputHandler<NavigationListeners, ButtonListeners>{
        navigationListeners, buttonListeners};
}

/// @return true if the menu should remain enabled, false if it should be
/// disabled
using MenuEntryCallback = bool (*)();
struct MenuEntry
{
    const char* label{};
    MenuEntryCallback onPress{};
};

template<typename... Entries>
struct MenuEntries
{
    constexpr MenuEntries(Entries... entries)
        : entries{entries...}
    {
    }
    static constexpr size_t size = sizeof...(Entries);
    MenuEntry entries[size];
};

template<typename... Entries>
constexpr MenuEntries<Entries...> makeMenuEntries(Entries... entries)
{
    return MenuEntries<Entries...>{entries...};
}

/// Menu with a list of entries and a callback when the menu is exited
/// Needs to know the TFT_eSPI object to draw the menu on the screen
template<typename TFT_eSPI, typename Entries>
class Menu
{
public:
    Menu(TFT_eSPI& tft, Entries entries, KeyColors menuColors)
        : tft_{tft}
        , entries_{entries}
        , labelColor_{makeColor(menuColors.label)}
        , unSelectedColor_{makeColor(menuColors.unpressed)}
        , selectedColor_{makeColor(menuColors.pressed)}
    {
    }

    void enableMenu(bool enable)
    {
        enabled_ = enable;
    }

    void begin(RectangleDimensions rect)
    {
        rect_ = rect;
        enableMenu(true);
    }

    void handleNavigationEvent(NavigationEvent event)
    {
        if (!enabled_)
        {
            return;
        }
        int previousIndex = selectedIndex_;
        switch (event)
        {
        case NavigationEvent::Up:
            selectedIndex_
                = (selectedIndex_ - 1 + entries_.size) % entries_.size;
            break;
        case NavigationEvent::Down:
            selectedIndex_ = (selectedIndex_ + 1) % entries_.size;
            break;
        case NavigationEvent::Press:
            if (selectedIndex_ < entries_.size)
            {
                const auto& entry = entries_.entries[selectedIndex_];
                enableMenu(entry.onPress());
            }
            break;
        default:
            break;
        }
        if (previousIndex != selectedIndex_)
        {
            drawSelectedEntry(previousIndex);
        }
    }

    void draw()
    {
        tft_.fillRoundRect(rect_.x0,
                           rect_.y0,
                           rect_.width,
                           rect_.height,
                           rect_.radius,
                           unSelectedColor_);
        tft_.drawRoundRect(rect_.x0,
                           rect_.y0,
                           rect_.width,
                           rect_.height,
                           rect_.radius,
                           labelColor_); // Outline effect
        tft_.setTextSize(2);
        tft_.setTextDatum(MC_DATUM);
        tft_.setTextPadding(getTextPadding() * 2);
        for (size_t i = 0; i < entries_.size; ++i)
        {
            const auto& entry = entries_.entries[i];
            int y = rect_.y0 + (i + 1) * rect_.height / (entries_.size + 1);
            if (i == selectedIndex_)
            {
                tft_.setTextColor(labelColor_, selectedColor_);
                tft_.drawString(entry.label, rect_.x0 + rect_.width / 2, y);
            }
            else
            {
                tft_.setTextColor(labelColor_, unSelectedColor_);
                tft_.drawString(entry.label, rect_.x0 + rect_.width / 2, y);
            }
        }
    }

    void drawSelectedEntry(int previousIndex)
    {
        // Redraw the previously selected entry with unselected color
        const auto& previousEntry = entries_.entries[previousIndex];
        int previousY
            = rect_.y0
              + (previousIndex + 1) * rect_.height / (entries_.size + 1);
        tft_.setTextSize(2);
        tft_.setTextDatum(MC_DATUM);
        tft_.setTextPadding(getTextPadding() * 2);
        tft_.setTextColor(labelColor_, unSelectedColor_);
        tft_.drawString(
            previousEntry.label, rect_.x0 + rect_.width / 2, previousY);

        // Draw the newly selected entry with selected color
        const auto& newEntry = entries_.entries[selectedIndex_];
        int newY             = rect_.y0
                   + (selectedIndex_ + 1) * rect_.height / (entries_.size + 1);
        tft_.setTextColor(labelColor_, selectedColor_);
        tft_.drawString(newEntry.label, rect_.x0 + rect_.width / 2, newY);
    }

private:
    TFT_eSPI& tft_;
    Entries entries_;
    int32_t labelColor_;
    int32_t unSelectedColor_;
    int32_t selectedColor_;
    RectangleDimensions rect_{};
    size_t selectedIndex_{0};
    bool enabled_{false};

    int getTextPadding() const
    {
        // Calculate the maximum width of the menu entries to determine the text
        // padding Go through the entries and find the max width of the labels
        // using strlen
        int maxWidth = 0;
        for (size_t i = 0; i < entries_.size; ++i)
        {
            const auto& entry = entries_.entries[i];
            int width         = strlen(entry.label);
            if (width > maxWidth)
            {
                maxWidth = width;
            }
        }
        return maxWidth;
    }
};

template<typename TFT_eSPI, typename Entries>
Menu<TFT_eSPI, Entries>
makeMenu(TFT_eSPI& tft, Entries entries, KeyColors menuColors)
{
    return Menu<TFT_eSPI, Entries>{tft, entries, menuColors};
}
