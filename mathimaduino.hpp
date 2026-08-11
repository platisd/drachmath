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

template<typename T>
constexpr T clamp0(T value)
{
    return value < 0 ? 0 : value;
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
template<typename TFT_eSPI, typename Listeners, typename SettingsHolderT>
class MathsQuiz
{
public:
    MathsQuiz(TFT_eSPI& tft,
              TftColor backgroundColor,
              TftColor textColor,
              Listeners listeners,
              SettingsHolderT& settingsHolder)
        : tft_{tft}
        , backgroundColor_{makeColor(backgroundColor)}
        , textColor_{makeColor(textColor)}
        , listeners_{listeners}
        , settingsHolder_{settingsHolder}
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

    /// @return true on exit/transition, false otherwise
    bool handleButtonEvent(ButtonEvent event)
    {
        if (!enabled_)
        {
            return false;
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
            return false; // Stay in the quiz
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
            return false; // Stay in the quiz
        case ButtonEvent::Right:
        {
            // Exit the quiz and return to the menu
            enableQuiz(false);
        }
            return true; // Exit the quiz
        default:
            return false; // Stay in the quiz
        }
    }

private:
    TFT_eSPI& tft_;
    int32_t backgroundColor_;
    int32_t textColor_;
    Listeners listeners_;
    SettingsHolderT& settingsHolder_;
    RectangleDimensions rect_{};
    int currentCorrectAnswer_{};
    bool enabled_{false};

    constexpr static int minOperand               = 1;
    constexpr static unsigned long operationsSize = 4;
    const char operations[operationsSize]         = {'+', '-', '*', '/'};
    /// Enough to contain something like "nnn + nnn = nnnnn" + null terminator
    constexpr static size_t questionBufferSize = 18;
    char questionBuffer_[questionBufferSize]   = {'\0'};
    size_t userAnswerIndex_{}; // Index of user answer in questionBuffer_

    MathsQuestion generateQuestion()
    {
        for (int attempt{0}; attempt < 1000; ++attempt)
        {
            MathsQuestion question = generateQuestionHelper();
            if (question.answer <= settingsHolder_.getMaxResult())
            {
                return question;
            }
        }
        return MathsQuestion{1, 1, '+', 2}; // Fallback
    }

    MathsQuestion generateQuestionHelper()
    {
        int maxOperand = settingsHolder_.getMaxOperand();
        int operand1   = random(minOperand, maxOperand + 1);
        int operand2   = random(minOperand, maxOperand + 1);
        char operation
            = operations[random(0, settingsHolder_.getOperationsCount())];
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
            // Let's make things easier for the kids
            if (operand1 > 10 && operand2 > 10)
            {
                operand2 = random(minOperand, 11);
            }
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

template<typename TFT_eSPI, typename Listeners, typename SettingsHolderT>
MathsQuiz<TFT_eSPI, Listeners, SettingsHolderT>
makeMathsQuiz(TFT_eSPI& tft,
              TftColor backgroundColor,
              TftColor textColor,
              Listeners listeners,
              SettingsHolderT& settingsHolder)
{
    return MathsQuiz<TFT_eSPI, Listeners, SettingsHolderT>{
        tft, backgroundColor, textColor, listeners, settingsHolder};
}

template<typename TFT_eSPI>
class Label
{
public:
    Label(TFT_eSPI& tft,
          TftColor backgroundColor,
          TftColor textColor,
          TftColor clearColor,
          int textSize)
        : tft_{tft}
        , backgroundColor_{makeColor(backgroundColor)}
        , textColor_{makeColor(textColor)}
        , clearColor_{makeColor(clearColor)}
        , textSize_{textSize}
    {
    }

    void begin(Point point)
    {
        point_ = point;
    }

    void clear()
    {
        if (labelBuffer_[0] == '\0')
        {
            return;
        }
        tft_.setTextSize(textSize_);
        int textWidth  = tft_.textWidth(labelBuffer_);
        int textHeight = tft_.fontHeight() * textSize_;
        int x0         = clamp0(point_.x - textWidth / 2);
        int y0         = clamp0(point_.y - textHeight / 2);
        tft_.fillRect(x0, y0, textWidth, textHeight, clearColor_);
        labelBuffer_[0] = '\0';
    }

    void draw(const char* label)
    {
        tft_.setTextColor(textColor_, backgroundColor_);
        tft_.setTextSize(textSize_);
        tft_.setTextPadding(0);
        tft_.setTextDatum(MC_DATUM);
        tft_.drawString(label, point_.x, point_.y);
        strncpy(labelBuffer_, label, maxLabelLength - 1);
        labelBuffer_[maxLabelLength - 1] = '\0';
    }

private:
    TFT_eSPI& tft_;
    int32_t backgroundColor_;
    int32_t textColor_;
    int32_t clearColor_;
    int textSize_;
    Point point_{};
    constexpr static size_t maxLabelLength = 5;
    char labelBuffer_[maxLabelLength]      = {'\0'};
};

template<typename TFT_eSPI>
Label<TFT_eSPI> makeLabel(TFT_eSPI& tft,
                          TftColor backgroundColor,
                          TftColor textColor,
                          TftColor clearColor,
                          int textSize)
{
    return Label<TFT_eSPI>{
        tft, backgroundColor, textColor, clearColor, textSize};
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
    MenuEntry(
        const char* label, MenuEntryCallback onPress = [] { return true; })
        : label{label}
        , onPress{onPress}
    {
    }
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

    void handleButtonEvent(ButtonEvent event)
    {
        if (!enabled_)
        {
            return;
        }
        // We only care about the left button which is also serves as select
        if (event == ButtonEvent::Left)
        {
            if (selectedIndex_ < entries_.size)
            {
                const auto& entry = entries_.entries[selectedIndex_];
                enableMenu(entry.onPress());
            }
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
        tft_.setTextPadding(getTextPadding());
        for (size_t i = 0; i < entries_.size; ++i)
        {
            const auto& entry = entries_.entries[i];
            int y = rect_.y0 + (i + 1) * rect_.height / (entries_.size + 1);
            if (i == selectedIndex_)
            {
                tft_.setTextColor(labelColor_, selectedColor_);
            }
            else
            {
                tft_.setTextColor(labelColor_, unSelectedColor_);
            }
            tft_.drawString(entry.label, rect_.x0 + rect_.width / 2, y);
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
        tft_.setTextPadding(getTextPadding());
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

    /// @return the width in pixels of the widest entry label
    int getTextPadding() const
    {
        int maxWidth = 0;
        for (size_t i = 0; i < entries_.size; ++i)
        {
            const auto& entry = entries_.entries[i];
            const int width   = tft_.textWidth(entry.label);
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

/// MenuEntryConfig is a class that holds the values of a MenuEntry that can be
/// configured by the user Contains an unknown number of options in an array and
/// a current index that points to the currently selected option
constexpr size_t maxMenuEntryConfigOptions = 4;
struct MenuEntryConfig
{
    const char* options[maxMenuEntryConfigOptions]{};
    size_t size{};
    size_t currentIndex{};
};

template<typename... Options>
constexpr MenuEntryConfig makeMenuEntryConfig(Options... options)
{
    static_assert(sizeof...(Options) <= maxMenuEntryConfigOptions,
                  "Too many options; increase maxMenuEntryConfigOptions");
    return MenuEntryConfig{{options...}, sizeof...(Options), 0};
}

/// Knows how to convert
using SettingsLogger = void (*)(const char* value);

struct SettingsEntry
{
    SettingsEntry(MenuEntry entry,
                  MenuEntryConfig config,
                  const char* persistentFilePath,
                  SettingsLogger logger)
        : entry{entry}
        , config{config}
        , persistentFilePath{persistentFilePath}
        , log{logger}
    {
        // Ensure the settings holder is initialized with a default value
        log(config.options[config.currentIndex]);
    }

    MenuEntry entry;
    MenuEntryConfig config;
    const char* persistentFilePath;
    SettingsLogger log;
};

template<typename... Entries>
struct SettingsEntries
{
    constexpr SettingsEntries(Entries... entries)
        : entries{entries...}
    {
    }
    static constexpr size_t size = sizeof...(Entries);
    SettingsEntry entries[size];
};

template<typename... Entries>
constexpr SettingsEntries<Entries...> makeSettingsEntries(Entries... entries)
{
    return SettingsEntries<Entries...>{entries...};
}

enum class Language
{
    Greek,
    English
};

// Creating a generic SettingsHolder was a fun excercise but let's
// keep it simple and implement one that is purpose-specific and encapsulates
// all relevant conversion logic.
class SettingsHolder
{
public:
    int getMaxOperand() const
    {
        return maxOperandValue_;
    }

    void setMaxOperand(const char* value)
    {
        maxOperandValue_ = atoi(value);
    }

    int getMaxResult() const
    {
        return maxResultValue_;
    }

    void setMaxResult(const char* value)
    {
        maxResultValue_ = atoi(value);
    }

    int getOperationsCount() const
    {
        return operationsCount_;
    }

    void setOperationsCount(const char* value)
    {
        int maybeCount = atoi(value);
        if (maybeCount > 0) // 0 or negative are not valid counts
        {
            operationsCount_ = maybeCount;
            return;
        }
        // Assuming value is a string of operations like "+-*/"
        operationsCount_ = strlen(value);
    }

    Language getLanguage() const
    {
        return language_;
    }

    void setLanguage(const char* value)
    {
        if (strcmp(value, "Greek") == 0 || strcmp(value, "0") == 0)
        {
            language_ = Language::Greek;
        }
        else if (strcmp(value, "English") == 0 || strcmp(value, "1") == 0)
        {
            language_ = Language::English;
        }
        else
        {
            language_ = Language::Greek;
        }
    }

    int getMaxWordLength() const
    {
        return maxWordLength_;
    }

    void setMaxWordLength(const char* value)
    {
        maxWordLength_ = atoi(value);
    }

private:
    int maxOperandValue_{};
    int maxResultValue_{};
    int operationsCount_{};
    Language language_{};
    int maxWordLength_{};
};

template<typename TFT_eSPI, typename Settings>
class SettingsMenu
{
public:
    SettingsMenu(TFT_eSPI& tft, Settings& settings, KeyColors menuColors)
        : tft_{tft}
        , settings_{settings}
        , labelColor_{makeColor(menuColors.label)}
        , unSelectedColor_{makeColor(menuColors.unpressed)}
        , selectedColor_{makeColor(menuColors.pressed)}
    {
    }

    void enableSettingsMenu(bool enable)
    {
        enabled_ = enable;
    }

    void begin(RectangleDimensions rect)
    {
        rect_ = rect;
        enableSettingsMenu(true);
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
        // Draw each entry and config. Entry aligned to the left, config aligned
        // to the right
        for (size_t i = 0; i < settings_.size; ++i)
        {
            drawEntry(i, i == selectedIndex_);
        }
    }

    void drawSelectedEntry(int previousIndex)
    {
        tft_.setTextSize(2);
        // Redraw the previously selected entry with unselected color
        drawEntry(previousIndex, false);
        // Draw the newly selected entry with selected color
        drawEntry(selectedIndex_, true);
    }

    void handleNavigationEvent(NavigationEvent event)
    {
        if (!enabled_)
        {
            return;
        }
        int previousIndex   = selectedIndex_;
        bool configChanged  = false;
        auto& currentEntry  = settings_.entries[selectedIndex_];
        auto& currentConfig = currentEntry.config;
        switch (event)
        {
        case NavigationEvent::Up:
            selectedIndex_
                = (selectedIndex_ - 1 + settings_.size) % settings_.size;
            break;
        case NavigationEvent::Down:
            selectedIndex_ = (selectedIndex_ + 1) % settings_.size;
            break;
        case NavigationEvent::Left:
            // Wrap around to the last option if at the first
            currentConfig.currentIndex
                = (currentConfig.currentIndex - 1 + currentConfig.size)
                  % currentConfig.size;
            configChanged = true;
            currentEntry.log(currentConfig.options[currentConfig.currentIndex]);
            break;
        case NavigationEvent::Right:
            // Wrap around to the first option if at the last
            currentConfig.currentIndex
                = (currentConfig.currentIndex + 1) % currentConfig.size;
            currentEntry.log(currentConfig.options[currentConfig.currentIndex]);
            configChanged = true;
            break;
        default:
            break;
        }
        if (previousIndex != selectedIndex_ || configChanged)
        {
            drawSelectedEntry(previousIndex);
        }
    }

    /// @return true on exit, false otherwise
    bool handleButtonEvent(ButtonEvent event)
    {
        if (!enabled_)
        {
            return false;
        }
        // We only care about the right button which serves as exit
        if (event == ButtonEvent::Right)
        {
            enableSettingsMenu(false);
            return true; // Exit the settings menu
        }
        return false; // Stay in the settings menu
    }

private:
    TFT_eSPI& tft_;
    Settings& settings_;
    int32_t labelColor_;
    int32_t unSelectedColor_;
    int32_t selectedColor_;
    RectangleDimensions rect_{};
    size_t selectedIndex_{0};
    bool enabled_{false};
    // Enough to contain the longest option as `<option>` + null terminator
    constexpr static size_t optionBufferSize = 16;
    char optionBuffer_[optionBufferSize];

    void drawEntry(size_t index, bool selected)
    {
        const auto& setting = settings_.entries[index];
        const int y
            = rect_.y0 + (index + 1) * rect_.height / (settings_.size + 1);
        tft_.setTextColor(labelColor_,
                          selected ? selectedColor_ : unSelectedColor_);
        tft_.setTextDatum(ML_DATUM);
        // We want the label's padding to expand to the end of the entry's
        // rectangle so it highlights the entire entry including the option and
        // also to remove any artifacts from the option text when it changes.
        const auto fullEntryWidth = rect_.width - 5 - 5; // 5px padding on sides
        tft_.setTextPadding(fullEntryWidth);
        tft_.drawString(setting.entry.label, rect_.x0 + 5, y);
        // Draw the current config option aligned to the right
        tft_.setTextDatum(MR_DATUM);
        tft_.setTextPadding(0); // Label handles artifacts & highlighting
        formatOption(setting.config.options[setting.config.currentIndex]);
        tft_.setTextColor(labelColor_); // Label handles highlighting
        tft_.drawString(optionBuffer_, rect_.x0 + rect_.width - 5, y);
    }

    void formatOption(const char* option)
    {
        snprintf(optionBuffer_, optionBufferSize, "<%s>", option);
    }

    /// @return the width in pixels of the widest option of this config. Scoped
    /// to a single config, since an option only ever has to erase another
    /// option of the same entry. A padding covering the widest option of *any*
    /// entry would reach into the label of the entries with long labels
    int getOptionPadding(const MenuEntryConfig& config)
    {
        int maxWidth = 0;
        for (size_t i = 0; i < config.size; ++i)
        {
            formatOption(config.options[i]);
            const int width = tft_.textWidth(optionBuffer_);
            if (width > maxWidth)
            {
                maxWidth = width;
            }
        }
        return maxWidth;
    }
};

template<typename TFT_eSPI, typename Settings>
SettingsMenu<TFT_eSPI, Settings>
makeSettingsMenu(TFT_eSPI& tft, Settings& settings, KeyColors menuColors)
{
    return SettingsMenu<TFT_eSPI, Settings>{tft, settings, menuColors};
}

template<typename OnDestroy>
class ScopedDestructor
{
public:
    ScopedDestructor(OnDestroy onDestroy)
        : onDestroy_{onDestroy}
    {
    }

    ScopedDestructor(const ScopedDestructor&)            = delete;
    ScopedDestructor& operator=(const ScopedDestructor&) = delete;
    // We need this move constructor to allow returning a ScopedDestructor from
    // a function, i.e. makeScopedDestructor This wouldn't be needed if we used
    // a standard with proper copy elision and/or CTAD
    ScopedDestructor(ScopedDestructor&& other) noexcept
        : onDestroy_{other.onDestroy_}
        , active_{other.active_}
    {
        other.active_ = false;
    }

    ~ScopedDestructor()
    {
        if (active_)
        {
            onDestroy_();
        }
    }

private:
    OnDestroy onDestroy_;
    bool active_{true};
};

template<typename OnDestroy>
ScopedDestructor<OnDestroy> makeScopedDestructor(OnDestroy onDestroy)
{
    return ScopedDestructor<OnDestroy>{onDestroy};
}

template<typename FileSystem>
class FileWriter
{
public:
    FileWriter(const char* filename, FileSystem& fs)
        : filename_{filename}
        , fs_{fs}
    {
    }

    bool test()
    {
        auto file = fs_.open(filename_, 0x0B /* FILE_WRITE */);
        if (!file)
        {
            return false;
        }
        file.close();
        return true;
    }

    template<typename T>
    void print(const T& data)
    {
        auto file      = fs_.open(filename_, 0x0B /* FILE_WRITE */);
        auto closeFile = makeScopedDestructor(
            [&file]
            {
                if (file)
                {
                    file.close();
                }
            });
        if (file)
        {
            file.print(data);
        }
    }

private:
    const char* filename_;
    FileSystem& fs_;
};

template<typename FileSystem>
FileWriter<FileSystem> makeFileWriter(const char* filename, FileSystem& fs)
{
    return FileWriter<FileSystem>{filename, fs};
}

template<typename FileSystem, size_t BufferSize>
class FileReader
{
    static_assert(BufferSize >= 2, "Need room for a character and a NULL");

public:
    FileReader(const char* filename, FileSystem& fs)
        : filename_{filename}
        , fs_{fs}
    {
    }

    const char* readLine()
    {
        auto file      = fs_.open(filename_, 1 /* FILE_READ */);
        auto closeFile = makeScopedDestructor(
            [&file]
            {
                if (file)
                {
                    file.close();
                }
            });

        if (!file || !file.available())
        {
            return nullptr;
        }
        return readLineHelper(file);
    }

    const char* readRandomLine()
    {
        auto file      = fs_.open(filename_, 1 /* FILE_READ */);
        auto closeFile = makeScopedDestructor(
            [&file]
            {
                if (file)
                {
                    file.close();
                }
            });

        if (!file || !file.available())
        {
            return nullptr;
        }

        // Move to a random location in the file, go to the next line
        // and read that line.
        // If we hit an EOF, retry and go to the beginning as a last resort
        const auto fileSize = file.size();
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            const auto randomOffset = random(fileSize);
            file.seek(randomOffset);
            if (randomOffset != 0)
            {
                while (file.available() && file.read() != '\n')
                {
                }
            }
            if (file.available())
            {
                return readLineHelper(file);
            }
        }
        file.seek(0);
        return readLineHelper(file);
    }

private:
    const char* filename_;
    FileSystem& fs_;
    char buffer_[BufferSize]{'\0'};

    template<typename FileT>
    const char* readLineHelper(FileT& file)
    {
        if (!file || !file.available())
        {
            return nullptr;
        }
        size_t length = 0;
        while (file.available() && length < BufferSize - 1)
        {
            const auto c = file.read();
            if (c == '\n')
            {
                break;
            }
            buffer_[length++] = static_cast<char>(c);
        }
        while (length > 0 && buffer_[length - 1] == '\r')
        {
            --length;
        }
        buffer_[length] = '\0';
        return buffer_;
    }
};

template<typename FileSystem, size_t BufferSize>
FileReader<FileSystem, BufferSize> makeFileReader(const char* filename,
                                                  FileSystem& fs)
{
    return FileReader<FileSystem, BufferSize>{filename, fs};
}

template<typename SettingsHolderT, typename FileSystem, typename Settings>
class PersistentSettings
{
public:
    PersistentSettings(SettingsHolderT& settingsHolder,
                       FileSystem& fs,
                       Settings& settings)
        : settingsHolder_{settingsHolder}
        , fs_{fs}
        , settings_{settings}
    {
    }

    void save()
    {
        for (auto& entry : settings_.entries)
        {
            auto writer = makeFileWriter(entry.persistentFilePath, fs_);
            writer.print(entry.config.options[entry.config.currentIndex]);
        }
    }

    void load()
    {
        for (auto& entry : settings_.entries)
        {
            auto reader
                = makeFileReader<FileSystem, 16>(entry.persistentFilePath, fs_);
            auto value = reader.readLine();
            if (!value)
            {
                continue;
            }
            // Update the index of the config to match the loaded value
            for (size_t i = 0; i < entry.config.size; ++i)
            {
                if (strcmp(value, entry.config.options[i]) == 0)
                {
                    entry.config.currentIndex = i;
                    entry.log(value);
                    break;
                }
            }
        }
    }

private:
    SettingsHolderT& settingsHolder_;
    FileSystem& fs_;
    Settings& settings_;
};

template<typename SettingsHolderT, typename FileSystem, typename Settings>
PersistentSettings<SettingsHolderT, FileSystem, Settings>
makePersistentSettings(SettingsHolderT& settingsHolder,
                       FileSystem& fs,
                       Settings& settings)
{
    return PersistentSettings<SettingsHolderT, FileSystem, Settings>{
        settingsHolder, fs, settings};
}

/// Returns true if SD card initialized successfully, false otherwise
using SdCardInitializer = bool (*)();
/// Returns true if SD card is present, false otherwise
using SdCardPresentDetector = bool (*)();

template<typename TestFileWriter>
class SdCardChecker
{
public:
    SdCardChecker(TestFileWriter testFileWriter,
                  SdCardInitializer sdCardInitializer,
                  SdCardPresentDetector sdCardPresentDetector)
        : testFileWriter_{testFileWriter}
        , initializeSdCard_{sdCardInitializer}
        , isSdCardPresent_{sdCardPresentDetector}
    {
    }

    // Called in setup()
    void begin()
    {
        handleSdCardActivity(true);
    }

    // Called in loop()
    void handleSdCardActivity(bool forceCheck = false)
    {
        const auto currentTime = millis();
        if (!forceCheck && currentTime - lastCheckTime_ < debounceDelay_)
        {
            return;
        }
        lastCheckTime_ = currentTime;
        auto present   = isSdCardPresent_();
        if (present == sdCardPresent_)
        {
            return; // No change in SD card presence
        }
        sdCardPresent_ = present;
        if (!sdCardPresent_)
        {
            sdCardReadyToUse_ = false;
            return;
        }
        // If we can write to the SD card then we are good to go, no re-init
        if (testFileWriter_.test())
        {
            sdCardReadyToUse_ = true;
            return;
        }
        sdCardReadyToUse_ = initializeSdCard_();
    }

    bool isSdCardReadyToUse() const
    {
        return sdCardReadyToUse_;
    }

private:
    TestFileWriter testFileWriter_;
    SdCardInitializer initializeSdCard_;
    SdCardPresentDetector isSdCardPresent_;
    bool sdCardPresent_{false};
    bool sdCardReadyToUse_{false};
    unsigned long lastCheckTime_{0};
    constexpr static unsigned long debounceDelay_{333}; // milliseconds
};

template<typename TestFileWriter>
SdCardChecker<TestFileWriter>
makeSdCardChecker(TestFileWriter testFileWriter,
                  SdCardInitializer sdCardInitializer,
                  SdCardPresentDetector sdCardPresentDetector)
{
    return SdCardChecker<TestFileWriter>{
        testFileWriter, sdCardInitializer, sdCardPresentDetector};
}

enum class GreekSpellingProblemType : uint8_t
{
    None,
    Vowel,
    Diphthong,
    DoubleConsonant,
    Digraph
};

namespace greek
{
constexpr uint16_t alpha{u'α'};
constexpr uint16_t beta{u'β'};
constexpr uint16_t gamma{u'γ'};
constexpr uint16_t epsilon{u'ε'};
constexpr uint16_t eta{u'η'};
constexpr uint16_t iota{u'ι'};
constexpr uint16_t kappa{u'κ'};
constexpr uint16_t lambda{u'λ'};
constexpr uint16_t mu{u'μ'};
constexpr uint16_t nu{u'ν'};
constexpr uint16_t omicron{u'ο'};
constexpr uint16_t pi{u'π'};
constexpr uint16_t rho{u'ρ'};
constexpr uint16_t finalSigma{u'ς'};
constexpr uint16_t sigma{u'σ'};
constexpr uint16_t tau{u'τ'};
constexpr uint16_t upsilon{u'υ'};
constexpr uint16_t omega{u'ω'};

constexpr uint16_t alphaTonos{u'ά'};
constexpr uint16_t epsilonTonos{u'έ'};
constexpr uint16_t etaTonos{u'ή'};
constexpr uint16_t iotaTonos{u'ί'};
constexpr uint16_t omicronTonos{u'ό'};
constexpr uint16_t upsilonTonos{u'ύ'};
constexpr uint16_t omegaTonos{u'ώ'};

constexpr uint16_t iotaDialytika{u'ϊ'};
constexpr uint16_t upsilonDialytika{u'ϋ'};
constexpr uint16_t iotaDialytikaTonos{u'ΐ'};
constexpr uint16_t upsilonDialytikaTonos{u'ΰ'};

static_assert(alpha == 0x03B1 && omega == 0x03C9 && finalSigma == 0x03C2
                  && iotaDialytikaTonos == 0x0390
                  && upsilonDialytikaTonos == 0x03B0,
              "Source file is not being compiled as UTF-8");
} // namespace greek

constexpr size_t maxGreekWordLetters{15};

/// One decoded letter of a word
struct GreekChar
{
    uint16_t codepoint{0};  // As it appeared in the word, e.g. ά
    uint16_t base{0};       // Tone and final sigma folded away, e.g. α
    uint16_t byteOffset{0}; // Where this letter starts in the original word
    uint8_t byteLength{0};  // 2 for Greek, 1 for ASCII
    bool accented{false};   // Carried a tonos
    bool dialytika{false};  // Was ϊ ϋ ΐ ΰ
};

/// Reduce a codepoint to the letter it is a variant of
inline uint16_t foldGreek(uint16_t codepoint, bool& accented, bool& dialytika)
{
    switch (codepoint)
    {
    case greek::alphaTonos:
        accented = true;
        return greek::alpha;
    case greek::epsilonTonos:
        accented = true;
        return greek::epsilon;
    case greek::etaTonos:
        accented = true;
        return greek::eta;
    case greek::iotaTonos:
        accented = true;
        return greek::iota;
    case greek::omicronTonos:
        accented = true;
        return greek::omicron;
    case greek::upsilonTonos:
        accented = true;
        return greek::upsilon;
    case greek::omegaTonos:
        accented = true;
        return greek::omega;
    case greek::iotaDialytika:
    case greek::iotaDialytikaTonos:
        dialytika = true;
        return greek::iota;
    case greek::upsilonDialytika:
    case greek::upsilonDialytikaTonos:
        dialytika = true;
        return greek::upsilon;
    case greek::finalSigma:
        return greek::sigma;
    default:
        // E.g. a capital letter, a punctuation mark etc
        return codepoint;
    }
}

/// Decode a UTF-8 word into letters.
/// Returns the number of letters, or 0 if the word is longer than maxLetters
inline size_t decodeGreekWord(const char* word,
                              GreekChar (&letters)[maxGreekWordLetters])
{
    size_t count{0};
    size_t offset{0};
    while (word[offset] != '\0')
    {
        const auto lead = static_cast<uint8_t>(word[offset]);
        uint16_t codepoint{0};
        uint8_t byteLength{0};
        if (lead < 0x80)
        {
            codepoint  = lead;
            byteLength = 1;
        }
        else if ((lead & 0xE0) == 0xC0)
        {
            const auto next = static_cast<uint8_t>(word[offset + 1]);
            codepoint
                = static_cast<uint16_t>(((lead & 0x1F) << 6) | (next & 0x3F));
            byteLength = 2;
        }
        else
        {
            // Not a one- or two-byte character, so not something a Greek word
            // is made of. Skipping one byte resynchronises on the next lead
            // byte.
            ++offset;
            continue;
        }

        if (count == maxGreekWordLetters)
        {
            return 0;
        }

        auto& letter     = letters[count];
        letter.codepoint = codepoint;
        letter.accented  = false;
        letter.dialytika = false;
        letter.base = foldGreek(codepoint, letter.accented, letter.dialytika);
        letter.byteOffset = static_cast<uint16_t>(offset);
        letter.byteLength = byteLength;
        ++count;
        offset += byteLength;
    }
    return count;
}

/// The vowels with a homophonic counterpart, so blanking one leaves a genuine
/// choice. α is not among them: it only ever sounds like itself.
inline bool isAmbiguousVowel(const GreekChar& letter)
{
    switch (letter.base)
    {
    case greek::epsilon: // Sounds like αι
    case greek::omicron: // Sounds like ω
    case greek::omega:   // Sounds like ο
    case greek::iota:    // Sounds like η, υ, ει, οι
    case greek::eta:     // Sounds like ι, υ, ει, οι
    case greek::upsilon: // Sounds like ι, η, ει, οι
        return true;
    default:
        return false;
    }
}

/// The two-letter vowel graphemes with a homophonic counterpart: αι, ει, οι,
/// αυ and ευ. ου, υι and ηυ are deliberately not here; they have nothing to be
/// confused with, so they fall through and are scanned as single letters.
inline bool isVowelPair(const GreekChar& first, const GreekChar& second)
{
    switch (first.base)
    {
    case greek::alpha:   // αι, αυ
    case greek::epsilon: // ει, ευ
        return second.base == greek::iota || second.base == greek::upsilon;
    case greek::omicron: // οι
        return second.base == greek::iota;
    default:
        return false;
    }
}

/// A consonant written twice but pronounced once, so the single form is a
/// plausible misspelling of it and vice versa.
inline bool isDoubleConsonant(const GreekChar& first, const GreekChar& second)
{
    if (first.base != second.base)
    {
        return false;
    }
    switch (first.base)
    {
    case greek::lambda:
    case greek::rho:
    case greek::sigma:
    case greek::tau:
    case greek::pi:
    case greek::kappa:
    case greek::mu:
    case greek::nu:
    case greek::beta:
        return true;
    default:
        return false;
    }
}

/// γγ and γκ
inline bool isDigraph(const GreekChar* letters, size_t index)
{
    if (letters[index].base != greek::gamma)
    {
        return false;
    }
    const auto second = letters[index + 1].base;
    if (second == greek::gamma)
    {
        return true;
    }
    return second == greek::kappa && index > 0;
}

constexpr uint8_t toFlag(GreekSpellingProblemType type)
{
    return type == GreekSpellingProblemType::None
               ? uint8_t{0}
               : static_cast<uint8_t>(1u << (static_cast<uint8_t>(type) - 1u));
}

/// Return a bitmask with the problems that can be generated from the word.
inline uint8_t getGreekSpellingProblems(const char* word)
{
    GreekChar letters[maxGreekWordLetters];
    const auto count = decodeGreekWord(word, letters);

    uint8_t problems{0};
    for (size_t i = 0; i < count; /* advanced below */)
    {
        // Ignore letters with dialytika, not interesting to worth the effort
        if (letters[i].dialytika)
        {
            ++i;
            continue;
        }
        // Two-letter patterns consume both letters
        if (i + 1 < count && !letters[i + 1].dialytika
            && isVowelPair(letters[i], letters[i + 1]))
        {
            // A tonos on the first vowel means the two are pronounced apart
            // (πλάι, γάιδαρος) and there is no homophone. On the second vowel
            // it is just how the pair is accented (αύριο), so it still counts.
            if (!letters[i].accented)
            {
                problems |= toFlag(GreekSpellingProblemType::Diphthong);
            }
            i += 2;
        }
        else if (i + 1 < count && isDigraph(letters, i))
        {
            problems |= toFlag(GreekSpellingProblemType::Digraph);
            i += 2;
        }
        else if (i + 1 < count && isDoubleConsonant(letters[i], letters[i + 1]))
        {
            problems |= toFlag(GreekSpellingProblemType::DoubleConsonant);
            i += 2;
        }
        else if (isAmbiguousVowel(letters[i]))
        {
            problems |= toFlag(GreekSpellingProblemType::Vowel);
            ++i;
        }
        else
        {
            ++i;
        }
    }
    return problems;
}
