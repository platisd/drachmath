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

inline void playCoinTickSound()
{
    playTone(379, 10);
}

inline void playCoinSound()
{
    playTone(568, 60);  // A5  880 Hz
    playTone(379, 60);  // E6  1319 Hz
    playTone(284, 160); // A6  1760 Hz
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

/// Encode a unicode character as UTF-8 string (null terminated)
/// Returns the number of bytes written, excluding the null terminator
inline int encodeGreekChar(uint16_t codepoint, char (&encoded)[3])
{
    if (codepoint < 0x80)
    {
        encoded[0] = static_cast<char>(codepoint);
        encoded[1] = '\0';
        return 1;
    }
    if (codepoint < 0x800)
    {
        encoded[0] = static_cast<char>(0xC0 | (codepoint >> 6));
        encoded[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
        encoded[2] = '\0';
        return 2;
    }
    return 0;
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

enum class QuizType
{
    Maths,
    Spelling
};

struct QuizRecord
{
    int correct{};
    int wrong{};
};

// Used for English characters and strings
constexpr uint8_t defaultEngFont{1U};

/// std::is_same reimplementation to avoid including <type_traits>
template<typename A, typename B>
struct SameType
{
    static constexpr bool value = false;
};

template<typename T>
struct SameType<T, T>
{
    static constexpr bool value = true;
};

template<typename T, typename... Rest>
struct AllSameType
{
    static constexpr bool value = true;
};

template<typename T, typename U, typename... Rest>
struct AllSameType<T, U, Rest...>
{
    static constexpr bool value
        = SameType<T, U>::value && AllSameType<U, Rest...>::value;
};

/// Hacky way to enable the Greek font only for a particular
/// scope without loading/unloading it.
/// The precondition is that before entering the scope we must
/// have loaded the Greek font and not unloaded it yet.
class ScopedGreekFont
{
public:
    explicit ScopedGreekFont(TFT_eSPI& tft)
        : tft_{tft}
    {
        // Setting fontLoaded to true directly would work until
        // we set it after accidentally unloading the font, which
        // would probably make bad things happen.
        // gUnicode should generally never be NULL unless our unloading
        // the Greek font logic is wrong
        tft_.fontLoaded = (tft_.gUnicode != nullptr);
    }

    ~ScopedGreekFont()
    {
        tft_.fontLoaded = false; // Suspended is the resting state
    }
    ScopedGreekFont(const ScopedGreekFont&)            = delete;
    ScopedGreekFont& operator=(const ScopedGreekFont&) = delete;

private:
    TFT_eSPI& tft_;
};

template<typename KeyListenerT, typename... Listeners>
struct KeyboardListeners
{
    constexpr KeyboardListeners(KeyListenerT first, Listeners... listeners)
        : listeners{first, listeners...}
    {
        static_assert(AllSameType<KeyListenerT, Listeners...>::value,
                      "All listeners must have the same type");
    }

    static constexpr size_t size = 1 + sizeof...(Listeners);
    KeyListenerT listeners[size];
};

template<typename KeyListenerT, typename... Listeners>
constexpr KeyboardListeners<KeyListenerT, Listeners...>
makeKeyboardListeners(KeyListenerT first, Listeners... listeners)
{
    return KeyboardListeners<KeyListenerT, Listeners...>{first, listeners...};
}

template<typename LabelT, typename... Labels>
struct KeyLabels
{
    constexpr KeyLabels(LabelT first, Labels... labels)
        : labels{first, labels...}
    {
        static_assert(AllSameType<LabelT, Labels...>::value,
                      "All labels must have the same type");
    }
    using LabelType = LabelT;

    static constexpr size_t size = 1 + sizeof...(Labels);
    LabelT labels[size];
};

template<typename LabelT, typename... Labels>
constexpr KeyLabels<LabelT, Labels...> makeKeyLabels(LabelT first,
                                                     Labels... labels)
{
    return KeyLabels<LabelT, Labels...>{first, labels...};
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

    bool isEnabled() const
    {
        return enabled_;
    }

    void begin(const RectangleDimensions& rect)
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
        maybeSetFontSize(LabelType{});
        for (int row = 0; row < Rows; ++row)
        {
            for (int col = 0; col < Columns; ++col)
            {
                int index = row * Columns + col;
                if (index >= Labels::size)
                {
                    break;
                }
                auto label      = labels_.labels[index];
                int x           = rect_.x0 + col * keyWidth;
                int y           = rect_.y0 + row * keyHeight;
                bool isSelected = (row == selectedRow_ && col == selectedCol_);
                tft_.drawRoundRect(
                    x, y, keyWidth, keyHeight, rect_.radius, outlineColor_);
                if (isSelected)
                {
                    tft_.fillRoundRect(x + 1,
                                       y + 1,
                                       keyWidth - 2,
                                       keyHeight - 2,
                                       rect_.radius,
                                       pressedColor_);
                }
                tft_.setTextColor(labelColor_,
                                  isSelected ? pressedColor_ : unpressedColor_);
                drawLabel(label, x + keyWidth / 2, y + keyHeight / 2);
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
        maybeSetFontSize(LabelType{});
        if (prevIndex < Labels::size)
        {
            auto prevLabel = labels_.labels[prevIndex];
            tft_.setTextColor(labelColor_, unpressedColor_);
            drawLabel(prevLabel, prevX + keyWidth / 2, prevY + keyHeight / 2);
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
            auto newLabel = labels_.labels[newIndex];
            tft_.setTextColor(labelColor_, pressedColor_);
            drawLabel(newLabel, newX + keyWidth / 2, newY + keyHeight / 2);
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
                auto key = labels_.labels[index];
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
    using LabelType = typename Labels::LabelType;

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

    void maybeSetFontSize(char)
    {
        tft_.setTextSize(2); // Only relevant for drawChar
    }

    void maybeSetFontSize(uint16_t)
    {
        // Smooth unicode fonts are fixed size
    }

    void drawLabel(char label, int x, int y)
    {
        const auto textHalfWidth  = tft_.textWidth("1") / 2;
        const auto textHalfHeight = tft_.fontHeight() / 2;
        tft_.drawChar(label, x - textHalfWidth, y - textHalfHeight);
    }

    void drawLabel(uint16_t label, int x, int y)
    {
        char text[3]             = {'\0'};
        const auto encodedLength = encodeGreekChar(label, text);
        if (encodedLength > 0)
        {
            ScopedGreekFont greekFont{tft_};
            const auto textHalfWidth  = tft_.textWidth(text) / 2;
            const auto textHalfHeight = tft_.fontHeight() / 2;
            tft_.setTextPadding(0);
            tft_.setTextDatum(TC_DATUM);
            tft_.drawString(text,
                            x - textHalfWidth,
                            y - textHalfHeight); // Greek character
        }
    }
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

/// Called when an answer is given in a quiz
using QuizListener = void (*)(bool correct);

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

    bool isEnabled() const
    {
        return enabled_;
    }

    void begin(const RectangleDimensions& rect)
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
                        rect_.y0 + rect_.height / 5,
                        defaultEngFont);
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
                            rect_.y0 + rect_.height / 5,
                            defaultEngFont);
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
                if (settingsHolder_.getSound())
                {
                    playCorrectSound();
                }
                notifyQuizListeners(true);
                drawNewQuestion();
            }
            else
            {
                // Incorrect answer, clear the user input
                if (settingsHolder_.getSound())
                {
                    playWrongSound();
                }
                notifyQuizListeners(false);
                questionBuffer_[userAnswerIndex_] = '\0';
                tft_.setTextColor(textColor_, backgroundColor_);
                tft_.setTextSize(3);
                tft_.setTextDatum(MC_DATUM);
                tft_.setTextPadding(rect_.width);
                tft_.drawString(questionBuffer_,
                                rect_.x0 + rect_.width / 2,
                                rect_.y0 + rect_.height / 5,
                                defaultEngFont);
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
                                rect_.y0 + rect_.height / 5,
                                defaultEngFont);
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

    void notifyQuizListeners(bool correct)
    {
        for (const auto& listener : listeners_.listeners)
        {
            listener(correct);
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
        if (drawnArea_.width == 0)
        {
            return;
        }
        tft_.fillRect(drawnArea_.x0,
                      drawnArea_.y0,
                      drawnArea_.width,
                      drawnArea_.height,
                      clearColor_);
        drawnArea_ = {};
    }

    void draw(const char* label)
    {
        tft_.setTextColor(textColor_, backgroundColor_);
        tft_.setTextSize(textSize_);
        tft_.setTextPadding(0);
        tft_.setTextDatum(MC_DATUM);
        tft_.drawString(label, point_.x, point_.y, defaultEngFont);
        strncpy(labelBuffer_, label, maxLabelLength - 1);
        labelBuffer_[maxLabelLength - 1] = '\0';
        drawnArea_                       = calculateDrawnArea(label);
    }

    int getX() const
    {
        return point_.x;
    }

    int getY() const
    {
        return point_.y;
    }

    int getHeight() const
    {
        return tft_.fontHeight();
    }

private:
    TFT_eSPI& tft_;
    int32_t backgroundColor_;
    int32_t textColor_;
    int32_t clearColor_;
    int textSize_;
    Point point_{};
    RectangleDimensions drawnArea_{};
    constexpr static size_t maxLabelLength = 12; // xxxx + coin space + null
    char labelBuffer_[maxLabelLength]      = {'\0'};

    RectangleDimensions calculateDrawnArea(const char* label) const
    {
        const int textWidth  = tft_.textWidth(label, defaultEngFont);
        const int textHeight = tft_.fontHeight(defaultEngFont);
        int x0               = clamp0(point_.x - textWidth / 2);
        int y0               = clamp0(point_.y - textHeight / 2);
        // TFT_eSPI::drawString shifts the string back on-screen
        if (x0 + textWidth > tft_.width())
        {
            x0 = clamp0(tft_.width() - textWidth);
        }
        if (y0 + textHeight > tft_.height())
        {
            y0 = clamp0(tft_.height() - textHeight);
        }
        return RectangleDimensions{x0, y0, textWidth, textHeight, 0};
    }
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

template<typename TFT_eSPI>
void drawCoin(TFT_eSPI& tft, int x, int y, int radius)
{
    tft.fillCircle(x + 3, y + 3, radius, tft.color565(120, 90, 0));
    tft.fillCircle(x, y, radius, tft.color565(255, 210, 30));
    tft.drawCircle(x, y, radius, tft.color565(170, 130, 0));
    tft.drawCircle(x, y, radius - 3, tft.color565(220, 180, 80));

    tft.fillCircle(x - radius / 3,
                   y - radius / 3,
                   radius / 4,
                   tft.color565(255, 240, 160));
    tft.drawFastHLine(
        x - radius + 8, y - 2, radius * 2 - 16, tft.color565(200, 160, 20));
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

    void logAnswer(QuizType quizType, bool correct)
    {
        auto& record
            = (quizType == QuizType::Maths) ? mathsRecord_ : spellingRecord_;
        if (correct)
        {
            ++record.correct;
        }
        else
        {
            ++record.wrong;
        }
    }

    int getScore() const
    {
        return score_;
    }

    QuizRecord getMathsRecord() const
    {
        return mathsRecord_;
    }

    QuizRecord getSpellingRecord() const
    {
        return spellingRecord_;
    }

    void draw()
    {
        snprintf(scoreBuffer_, sizeof(scoreBuffer_), "%d  ", score_);
        label_.draw(scoreBuffer_);
        int coinRadius = (label_.getHeight() / 2) * 0.9F;
        drawCoin(tft_, tft_.width() - coinRadius, coinRadius, coinRadius);
    }

    void hide()
    {
        label_.clear();
    }

private:
    Lbl& label_;
    TFT_eSPI& tft_;
    int score_{0};
    QuizRecord mathsRecord_{};
    QuizRecord spellingRecord_{};
    char scoreBuffer_[16] = {'\0'};
};

template<typename Lbl, typename TFT_eSPI>
ScoreKeeper<Lbl, TFT_eSPI> makeScoreKeeper(Lbl& label, TFT_eSPI& tft)
{
    return ScoreKeeper<Lbl, TFT_eSPI>{label, tft};
}

/// Return true if the listener has consumed the event
/// and the event should not be propagated to other listeners
using ButtonListener = bool (*)(ButtonEvent);
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

/// Return true if the listener has consumed the event
/// and the event should not be propagated to other listeners
using NavigationListener = bool (*)(NavigationEvent);
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
                    const auto consumed = listener(lastNavigationEvent_);
                    if (consumed)
                    {
                        break;
                    }
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
                    const auto consumed = listener(lastButtonEvent_);
                    if (consumed)
                    {
                        break;
                    }
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
                    const auto consumed = listener(ButtonEvent::Right);
                    if (consumed)
                    {
                        break;
                    }
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

    bool isEnabled() const
    {
        return enabled_;
    }

    void begin(const RectangleDimensions& rect)
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
            tft_.drawString(
                entry.label, rect_.x0 + rect_.width / 2, y, defaultEngFont);
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
        tft_.drawString(previousEntry.label,
                        rect_.x0 + rect_.width / 2,
                        previousY,
                        defaultEngFont);

        // Draw the newly selected entry with selected color
        const auto& newEntry = entries_.entries[selectedIndex_];
        int newY             = rect_.y0
                   + (selectedIndex_ + 1) * rect_.height / (entries_.size + 1);
        tft_.setTextColor(labelColor_, selectedColor_);
        tft_.drawString(
            newEntry.label, rect_.x0 + rect_.width / 2, newY, defaultEngFont);
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

    void setSound(const char* value)
    {
        if (strcmp(value, "Off") == 0 || strcmp(value, "0") == 0)
        {
            sound_ = 0;
        }
        else
        {
            sound_ = 1; // Default to On
        }
    }

    int getSound() const
    {
        return sound_;
    }

private:
    int maxOperandValue_{};
    int maxResultValue_{};
    int operationsCount_{};
    Language language_{};
    int maxWordLength_{};
    int sound_{};
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

    void enableMenu(bool enable)
    {
        enabled_ = enable;
    }

    bool isEnabled() const
    {
        return enabled_;
    }

    void begin(const RectangleDimensions& rect)
    {
        rect_ = rect;
        enableMenu(true);
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
            enableMenu(false);
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
        tft_.drawString(setting.entry.label, rect_.x0 + 5, y, defaultEngFont);
        // Draw the current config option aligned to the right
        tft_.setTextDatum(MR_DATUM);
        tft_.setTextPadding(0); // Label handles artifacts & highlighting
        formatOption(setting.config.options[setting.config.currentIndex]);
        tft_.setTextColor(labelColor_); // Label handles highlighting
        tft_.drawString(
            optionBuffer_, rect_.x0 + rect_.width - 5, y, defaultEngFont);
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

template<size_t BufferSize, typename FileSystem>
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
            auto reader = makeFileReader<16>(entry.persistentFilePath, fs_);
            auto value  = reader.readLine();
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

constexpr size_t greekSpellingProblemTypeCount{3};
enum class GreekSpellingProblemType : uint8_t
{
    None,
    Vowel,
    Diphthong,
    Digraph
};

/// The set of letters a blanked spot can plausibly be filled with
enum class GreekHomophoneGroup : uint8_t
{
    None,
    EpsilonSound, // ε, αι
    IotaSound,    // ι, η, υ, ει, οι
    OmicronSound, // ο, ω
    AvSound,      // αυ, αβ, αφ
    EvSound,      // ευ, εβ, εφ
    GammaNasal    // γγ, γκ
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
constexpr uint16_t phi{u'φ'};
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
    uint16_t codepoint{0}; // As it appeared in the word, e.g. ά
    uint16_t base{0};      // Tone and final sigma folded away, e.g. α
    int byteOffset{0};     // Where this letter starts in the original word
    int byteLength{0};     // 2 for Greek, 1 for ASCII
    bool accented{false};  // Carried a tonos
    bool dialytika{false}; // Was ϊ ϋ ΐ ΰ
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
        int byteLength{0};
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

inline bool isOuPair(const GreekChar& first, const GreekChar& second)
{
    return first.base == greek::omicron && second.base == greek::upsilon;
}

/// The two-letter vowel graphemes with a homophonic counterpart: αι, ει, οι,
/// αυ and ευ. υι and ηυ are to be treated as individual vowels, while ου is too
/// easy so treated separately
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

/// The homophones a one- or two-letter spot can be filled from. Reads the
/// folded bases, so tones and final sigma are already out of the way.
inline GreekHomophoneGroup
getHomophoneGroup(const GreekChar* letters, size_t index, uint8_t letterCount)
{
    const auto first = letters[index].base;
    if (letterCount == 2)
    {
        const auto second = letters[index + 1].base;
        switch (first)
        {
        case greek::alpha: // αι, αυ
            return second == greek::iota ? GreekHomophoneGroup::EpsilonSound
                                         : GreekHomophoneGroup::AvSound;
        case greek::epsilon: // ει, ευ
            return second == greek::iota ? GreekHomophoneGroup::IotaSound
                                         : GreekHomophoneGroup::EvSound;
        case greek::omicron: // οι
            return GreekHomophoneGroup::IotaSound;
        case greek::gamma: // γγ, γκ
            return GreekHomophoneGroup::GammaNasal;
        default:
            return GreekHomophoneGroup::None;
        }
    }
    switch (first)
    {
    case greek::epsilon:
        return GreekHomophoneGroup::EpsilonSound;
    case greek::omicron:
    case greek::omega:
        return GreekHomophoneGroup::OmicronSound;
    case greek::iota:
    case greek::eta:
    case greek::upsilon:
        return GreekHomophoneGroup::IotaSound;
    default:
        return GreekHomophoneGroup::None;
    }
}

constexpr uint8_t toFlag(GreekSpellingProblemType type)
{
    return type == GreekSpellingProblemType::None
               ? uint8_t{0}
               : static_cast<uint8_t>(1u << (static_cast<uint8_t>(type) - 1u));
}

constexpr size_t toIndex(GreekSpellingProblemType type)
{
    // None has no slot of its own
    return static_cast<size_t>(type) - 1u;
}

/// Where to blank a word and what the blank can be filled with. The offsets
/// index into the word that was handed to getGreekSpellingProblems, so nothing
/// here owns a string.
struct GreekSpellingCandidate
{
    GreekHomophoneGroup group{GreekHomophoneGroup::None};
    int byteOffset{0}; // Where the problem starts in the original word
    int byteLength{0}; // 2 for one Greek letter, 4 for two
};

struct GreekSpellingProblems
{
    GreekSpellingCandidate byType[greekSpellingProblemTypeCount]{};
    uint8_t available{0}; // Bitmask of toFlag() values

    bool has(GreekSpellingProblemType type) const
    {
        return (available & toFlag(type)) != 0;
    }

    const GreekSpellingCandidate& get(GreekSpellingProblemType type) const
    {
        return byType[toIndex(type)];
    }
};

/// Choose a candidate of the given type to offer as a spelling problem.
/// If there are multiple candidates of the same type, each has an equal chance
/// of being offered.
inline void offerSpellingCandidate(GreekSpellingProblems& problems,
                                   int (&seen)[greekSpellingProblemTypeCount],
                                   GreekSpellingProblemType type,
                                   const GreekChar* letters,
                                   size_t index,
                                   int letterCount)
{
    const auto slot = toIndex(type);
    ++seen[slot];
    // Keep the first spot, then replace the one held with probability 1/seen
    if (random(seen[slot]) != 0)
    {
        return;
    }
    const auto gapLength
        = letterCount == 2
              ? letters[index].byteLength + letters[index + 1].byteLength
              : letters[index].byteLength;
    problems.available |= toFlag(type);
    problems.byType[slot]
        = GreekSpellingCandidate{getHomophoneGroup(letters, index, letterCount),
                                 letters[index].byteOffset,
                                 gapLength};
}

/// Return the problems that can be generated from the word.
inline GreekSpellingProblems getGreekSpellingProblems(const char* word)
{
    GreekChar letters[maxGreekWordLetters];
    const auto count = decodeGreekWord(word, letters);

    GreekSpellingProblems problems{};
    // How many spots of each type we have come across so far
    int seen[greekSpellingProblemTypeCount]{};
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
            && isOuPair(letters[i], letters[i + 1]))
        {
            i += 2; // Ignore ου, no homophone and obvious as a vowel pair
        }
        else if (i + 1 < count && !letters[i + 1].dialytika
                 && isVowelPair(letters[i], letters[i + 1]))
        {
            // A tonos on the first vowel means the two are pronounced apart
            // (πλάι, γάιδαρος) and there is no homophone. On the second vowel
            // it is just how the pair is accented (αύριο), so it still counts.
            if (!letters[i].accented)
            {
                offerSpellingCandidate(problems,
                                       seen,
                                       GreekSpellingProblemType::Diphthong,
                                       letters,
                                       i,
                                       2);
            }
            i += 2;
        }
        else if (i + 1 < count && isDigraph(letters, i))
        {
            offerSpellingCandidate(problems,
                                   seen,
                                   GreekSpellingProblemType::Digraph,
                                   letters,
                                   i,
                                   2);
            i += 2;
        }
        else if (isAmbiguousVowel(letters[i]))
        {
            offerSpellingCandidate(
                problems, seen, GreekSpellingProblemType::Vowel, letters, i, 1);
            ++i;
        }
        else
        {
            ++i;
        }
    }
    return problems;
}

using KeyboardDrawer = void (*)(GreekHomophoneGroup);

/// GreekSpellingQuiz
template<typename TFT_eSPI,
         typename Listeners,
         typename SettingsHolderT,
         typename FileReaderT>
class GreekSpellingQuiz
{
public:
    GreekSpellingQuiz(TFT_eSPI& tft,
                      Listeners& listeners,
                      SettingsHolderT& settingsHolder,
                      FileReaderT& fileReader,
                      TftColor backgroundColor,
                      TftColor textColor)
        : tft_{tft}
        , listeners_{listeners}
        , settingsHolder_{settingsHolder}
        , fileReader_{fileReader}
        , backgroundColor_{makeColor(backgroundColor)}
        , textColor_{makeColor(textColor)}
    {
    }

    void enableQuiz(bool enable)
    {
        enabled_ = enable;
    }

    bool isEnabled() const
    {
        return enabled_;
    }

    void begin(const RectangleDimensions& rect)
    {
        rect_ = rect;
        enableQuiz(true);
    }

    void drawNewQuestion()
    {
        userInput_[0] = '\0';
        if (!selectValidWordAndProblem())
        {
            setFallbackQuestion();
        }
        drawWord();
        if (drawAppropriateKeyboard_)
        {
            drawAppropriateKeyboard_(currentProblem_.group);
        }
    }

    void handleKeyboardPress(uint16_t greekChar)
    {
        if (!enabled_)
        {
            return;
        }
        char encoded[3]{'\0'};
        const auto encodedLength = encodeGreekChar(greekChar, encoded);
        if (encodedLength == 0)
        {
            return; // Invalid character?!
        }
        const auto currentLength = strlen(userInput_);
        if (currentLength + encodedLength > maxUserInputLength)
        {
            return;
        }
        // Append the new character to the user input
        memcpy(userInput_ + currentLength, encoded, encodedLength);
        userInput_[currentLength + encodedLength] = '\0';
        // Update the displayed word with the user's input
        prepareDisplayedWord();
        drawWord();
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
            if (isAnswerCorrect())
            {
                if (settingsHolder_.getSound())
                {
                    playCorrectSound();
                }
                notifyQuizListeners(true);
                drawNewQuestion();
            }
            else
            {
                if (settingsHolder_.getSound())
                {
                    playWrongSound();
                }
                notifyQuizListeners(false);
                userInput_[0] = '\0';
                prepareDisplayedWord();
                drawWord();
            }
            return false; // Stay in the quiz
        case ButtonEvent::Middle:
        {
            const auto currentLength = static_cast<int>(strlen(userInput_));
            if (currentLength == 0)
            {
                return false;
            }
            // Remove the last character, which may be 1 or 2 bytes
            const int lastCharLength
                = (userInput_[currentLength - 1] & 0x80) == 0 ? 1 : 2;
            const size_t newLength = clamp0(currentLength - lastCharLength);
            userInput_[newLength]  = '\0';
            prepareDisplayedWord();
            drawWord();
        }
            return false;
        case ButtonEvent::Right:
            // Exit the quiz and return to the menu
            enableQuiz(false);
            return true; // Exit the quiz
        default:
            return false;
        }
    }

    GreekHomophoneGroup getCurrentProblemHomophone() const
    {

        return currentProblem_.group;
    }

    /// Must be called before drawing the first question so that the right
    /// keyboard can be picked based on the homophone group of each question.
    void registerKeyboardDrawer(KeyboardDrawer picker)
    {
        drawAppropriateKeyboard_ = picker;
    }

private:
    TFT_eSPI& tft_;
    Listeners& listeners_;
    SettingsHolderT& settingsHolder_;
    FileReaderT& fileReader_;
    KeyboardDrawer drawAppropriateKeyboard_{nullptr};
    int32_t backgroundColor_;
    int32_t textColor_;
    bool enabled_{false};
    RectangleDimensions rect_{};

    /// The randomly chosen word
    char currentWord_[maxGreekWordLetters * 2 + 1]{'\0'};
    constexpr static size_t maxUserInputLength{4}; // 2 Greek letters, 4 bytes
    char displayedWord_[maxGreekWordLetters * 2 + maxUserInputLength + 1]{'\0'};
    GreekSpellingCandidate currentProblem_{};
    char userInput_[maxUserInputLength + 1]{'\0'};

    bool isAnswerCorrect() const
    {
        const auto gapLength = static_cast<size_t>(currentProblem_.byteLength);
        return strlen(userInput_) == gapLength
               && memcmp(userInput_,
                         currentWord_ + currentProblem_.byteOffset,
                         gapLength)
                      == 0;
    }

    bool selectValidWordAndProblem()
    {
        for (int attempt = 0; attempt < 100; ++attempt)
        {
            const auto word = fileReader_.readRandomLine();
            if (!word)
            {
                continue;
            }
            // Two bytes per Greek character
            const auto maxWordBytes
                = 2 * static_cast<size_t>(settingsHolder_.getMaxWordLength());
            if (strlen(word) > maxWordBytes)
            {
                continue;
            }
            const auto problems = getGreekSpellingProblems(word);
            if (problems.available == 0)
            {
                continue;
            }

            const auto chosenType = chooseRandomProblemType(problems);
            currentProblem_       = problems.get(chosenType);

            strncpy(currentWord_, word, sizeof(currentWord_) - 1);
            currentWord_[sizeof(currentWord_) - 1] = '\0';

            prepareDisplayedWord();
            return true;
        }
        return false;
    }

    GreekSpellingProblemType
    chooseRandomProblemType(const GreekSpellingProblems& problems)
    {
        GreekSpellingProblemType chosenType{GreekSpellingProblemType::None};
        while (chosenType == GreekSpellingProblemType::None)
        {
            const auto randomIndex = random(greekSpellingProblemTypeCount);
            const auto type
                = static_cast<GreekSpellingProblemType>(randomIndex + 1);
            if (problems.has(type))
            {
                chosenType = type;
            }
        }
        return chosenType;
    }

    void prepareDisplayedWord()
    {
        const auto prefixLength
            = static_cast<size_t>(currentProblem_.byteOffset);
        const auto inputLength = strlen(userInput_);
        // The part of the word before the gap, unchanged
        memcpy(displayedWord_, currentWord_, prefixLength);
        // Then the gap, holding what the user has typed so far
        memcpy(displayedWord_ + prefixLength, userInput_, inputLength);
        auto offset = prefixLength + inputLength;
        // Show a single underscore while nothing has been typed to
        // avoid revealing the amount of missing letters
        // Once something's typed no gap is shown at all.
        if (inputLength == 0)
        {
            displayedWord_[offset++] = '_';
        }
        // Rest of the word, unchanged
        strncpy(displayedWord_ + offset,
                currentWord_ + prefixLength + currentProblem_.byteLength,
                sizeof(displayedWord_) - offset - 1);
        displayedWord_[sizeof(displayedWord_) - 1] = '\0';
    }

    void drawWord()
    {
        ScopedGreekFont greekFont{tft_};
        tft_.setTextColor(textColor_, backgroundColor_);
        // No need to set the size, the Greek font has fixed size
        tft_.setTextDatum(BC_DATUM);
        tft_.setTextPadding(rect_.width);
        tft_.drawString(displayedWord_,
                        rect_.x0 + rect_.width / 2,
                        rect_.y0 + rect_.height / 2); // Greek string
    }

    void setFallbackQuestion()
    {
        strncpy(currentWord_, "τυρί", sizeof(currentWord_) - 1);
        currentWord_[sizeof(currentWord_) - 1] = '\0';
        // The υ of τυρί, two bytes in
        currentProblem_
            = GreekSpellingCandidate{GreekHomophoneGroup::IotaSound, 2, 2};
        prepareDisplayedWord();
    }

    void notifyQuizListeners(bool correct)
    {
        for (const auto& listener : listeners_.listeners)
        {
            listener(correct);
        }
    }
};

template<typename TFT_eSPI,
         typename Listeners,
         typename SettingsHolderT,
         typename FileReaderT>
GreekSpellingQuiz<TFT_eSPI, Listeners, SettingsHolderT, FileReaderT>
makeGreekSpellingQuiz(TFT_eSPI& tft,
                      Listeners& listeners,
                      SettingsHolderT& settingsHolder,
                      FileReaderT& fileReader,
                      TftColor backgroundColor,
                      TftColor textColor)
{
    return GreekSpellingQuiz<TFT_eSPI, Listeners, SettingsHolderT, FileReaderT>{
        tft, listeners, settingsHolder, fileReader, backgroundColor, textColor};
}

template<typename TFT_eSPI, typename ScoreKeeperT, typename SettingsHolderT>
class StatsScreen
{
public:
    StatsScreen(TFT_eSPI& tft,
                ScoreKeeperT& scoreKeeper,
                SettingsHolderT& settingsHolder,
                TftColor backgroundColor,
                TftColor textColor)
        : tft_{tft}
        , scoreKeeper_{scoreKeeper}
        , settingsHolder_{settingsHolder}
        , backgroundColor_{makeColor(backgroundColor)}
        , textColor_{makeColor(textColor)}
    {
    }

    void begin(const RectangleDimensions& rect)
    {
        rect_ = rect;
        enableStatsScreen(true);
    }

    void enableStatsScreen(bool enabled)
    {
        enabled_ = enabled;
    }

    bool isEnabled() const
    {
        return enabled_;
    }

    void draw()
    {
        if (!enabled_)
        {
            return;
        }
        drawStaticContent();
        animateScoreCountUp();
    }

private:
    TFT_eSPI& tft_;
    ScoreKeeperT& scoreKeeper_;
    SettingsHolderT& settingsHolder_;
    int32_t backgroundColor_;
    int32_t textColor_;

    RectangleDimensions rect_{};
    bool enabled_{false};

    int coinX() const
    {
        return rect_.x0 + rect_.width / 2;
    }

    int coinY() const
    {
        return rect_.y0 + 38;
    }

    void drawScoreInCoin(int score)
    {
        char buf[6] = {'\0'}; // xxxxx + null terminator
        snprintf(buf, sizeof(buf), "%d", score);

        tft_.setTextColor(tft_.color565(110, 80, 0),
                          tft_.color565(255, 210, 30));
        tft_.setTextDatum(MC_DATUM);
        tft_.setTextSize(2);
        tft_.setTextPadding(0);
        tft_.drawString(buf, coinX() + 2, coinY(), defaultEngFont);
    }

    void drawStaticContent()
    {
        tft_.fillRect(
            rect_.x0, rect_.y0, rect_.width, rect_.height, backgroundColor_);
        tft_.drawRect(
            rect_.x0, rect_.y0, rect_.width, rect_.height, textColor_);
        // Top middle part of the rectangle draw the coin
        constexpr int coinRadius = 28;
        drawCoin(tft_, coinX(), coinY(), coinRadius);

        tft_.setTextColor(textColor_, backgroundColor_);
        tft_.setTextDatum(TL_DATUM);
        tft_.setTextSize(2);
        tft_.setTextPadding(0);
        char buf[32]              = {'\0'}; // Large enough for large scores
        const auto fontHeight     = tft_.fontHeight();
        const auto xOffset        = rect_.x0 + 10;
        const auto yOffset        = rect_.y0 + 80;
        const auto mathsRecord    = scoreKeeper_.getMathsRecord();
        const auto spellingRecord = scoreKeeper_.getSpellingRecord();
        snprintf(buf,
                 sizeof(buf),
                 "Maths: %d/%d",
                 mathsRecord.correct,
                 mathsRecord.correct + mathsRecord.wrong);
        tft_.drawString(buf, xOffset, yOffset, defaultEngFont);
        snprintf(buf,
                 sizeof(buf),
                 "Spelling: %d/%d",
                 spellingRecord.correct,
                 spellingRecord.correct + spellingRecord.wrong);
        tft_.drawString(buf, xOffset, yOffset + 2 * fontHeight, defaultEngFont);
    }

    void animateScoreCountUp()
    {
        const auto score = scoreKeeper_.getScore();
        if (score <= 0)
        {
            drawScoreInCoin(0);
            return;
        }
        const auto soundOn            = settingsHolder_.getSound();
        constexpr int maxCountUpSteps = 18;
        const auto steps = score < maxCountUpSteps ? score : maxCountUpSteps;
        for (int step = 1; step <= steps; ++step)
        {
            // Last frame lands exactly on the score despite rounding
            const int value
                = static_cast<int>((static_cast<long>(score) * step) / steps);
            drawScoreInCoin(value);
            if (soundOn)
            {
                playCoinTickSound();
            }
            delay(stepDelayMs(step, steps));
        }
        if (soundOn)
        {
            playCoinSound();
        }
    }

    static int stepDelayMs(int step, int steps)
    {
        // The last step lingers about twice as long as the first
        const long weight    = steps + step; // 1x .. 2x
        const long sumWeight = static_cast<long>(steps) * steps
                               + static_cast<long>(steps) * (steps + 1) / 2;
        // Budget for the delays of the whole count up, regardless of the score
        constexpr long countUpDurationMs = 1200;
        return static_cast<int>(countUpDurationMs * weight / sumWeight);
    }
};

template<typename TFT_eSPI, typename ScoreKeeperT, typename SettingsHolderT>
StatsScreen<TFT_eSPI, ScoreKeeperT, SettingsHolderT>
makeStatsScreen(TFT_eSPI& tft,
                ScoreKeeperT& scoreKeeper,
                SettingsHolderT& settingsHolder,
                TftColor backgroundColor,
                TftColor textColor)
{
    return StatsScreen<TFT_eSPI, ScoreKeeperT, SettingsHolderT>{
        tft, scoreKeeper, settingsHolder, backgroundColor, textColor};
}

// TODO: Lock settings with lock_settings.txt file
