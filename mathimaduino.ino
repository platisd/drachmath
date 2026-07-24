#include "TFT_eSPI.h"
#include "mathimaduino.hpp"

TFT_eSPI tft;

volatile NavigationEvent lastNavigationEvent   = NavigationEvent::None;
volatile unsigned long lastNavigationEventTime = 0;
volatile ButtonEvent lastButtonEvent           = ButtonEvent::None;
volatile unsigned long lastButtonEventTime     = 0;
constexpr unsigned long debounceDelay          = 50; // milliseconds
auto scoreLabel         = makeLabel(tft, colors::Black, colors::White, 2);
auto scoreKeeper        = makeScoreKeeper(scoreLabel, tft);
auto mathsQuizListeners = makeQuizListeners([] { scoreKeeper.increment(); });
auto mathsQuiz
    = makeMathsQuiz(tft, colors::Red, colors::White, mathsQuizListeners);
auto keyListeners = makeKeyboardListeners(
    [](char key) { mathsQuiz.handleKeyboardPress(key); });
auto keyLabels
    = makeKeyLabels('1', '2', '3', '4', '5', '6', '7', '8', '9', '0');
const KeyColors keyColors
    = {colors::White, colors::Black, colors::Green, colors::Gray};
auto keyboard = makeKeyboard<2, 5>(tft, keyColors, keyLabels, keyListeners);
auto leftButtonLabel   = makeLabel(tft, colors::Black, colors::White);
auto middleButtonLabel = makeLabel(tft, colors::Black, colors::White);
auto rightButtonLabel  = makeLabel(tft, colors::Black, colors::White);

void attachInterrupts()
{
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_UP),
        []
        {
            lastNavigationEvent     = NavigationEvent::Up;
            lastNavigationEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_DOWN),
        []
        {
            lastNavigationEvent     = NavigationEvent::Down;
            lastNavigationEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_LEFT),
        []
        {
            lastNavigationEvent     = NavigationEvent::Left;
            lastNavigationEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_RIGHT),
        []
        {
            lastNavigationEvent     = NavigationEvent::Right;
            lastNavigationEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_PRESS),
        []
        {
            lastNavigationEvent     = NavigationEvent::Press;
            lastNavigationEventTime = millis();
        },
        RISING);
    // Cannot attach an interrupt to both WIO_KEY_A and WIO_5S_UP
    // as they share the same external interrupt line. Poll instead.
    // attachInterrupt(
    //     digitalPinToInterrupt(WIO_KEY_A),
    //     []
    //     {
    //         lastButtonEvent     = ButtonEvent::Right;
    //         lastButtonEventTime = millis();
    //     },
    //     RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_KEY_B),
        []
        {
            lastButtonEvent     = ButtonEvent::Middle;
            lastButtonEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_KEY_C),
        []
        {
            lastButtonEvent     = ButtonEvent::Left;
            lastButtonEventTime = millis();
        },
        RISING);
}

void setup()
{
    randomSeed(analogRead(WIO_LIGHT));
    pinMode(WIO_5S_UP, INPUT_PULLUP);
    pinMode(WIO_5S_DOWN, INPUT_PULLUP);
    pinMode(WIO_5S_LEFT, INPUT_PULLUP);
    pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
    pinMode(WIO_5S_PRESS, INPUT_PULLUP);
    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    pinMode(WIO_BUZZER, OUTPUT);
    attachInterrupts();
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(3);

    tft.fillScreen(makeColor(colors::Red));
    const RectangleDimensions keyboardAtBottom{
        0, tft.height() * 2 / 3, tft.width(), tft.height() / 3, 5};
    keyboard.begin(keyboardAtBottom);
    keyboard.draw();
    const RectangleDimensions mathsQuizAboveKeyboardInTheMiddle{
        0, tft.height() / 3, tft.width(), tft.height() / 3, 5};
    mathsQuiz.begin(mathsQuizAboveKeyboardInTheMiddle);
    mathsQuiz.drawNewQuestion();
    // the button labels are on the top left of the screen and are a few pixels
    // large. They are roughly 10% of the screen away from each o ther
    leftButtonLabel.begin({tft.width() * 0.055F, 0});
    leftButtonLabel.draw("OK");
    middleButtonLabel.begin({tft.width() * 0.28F, 0});
    middleButtonLabel.draw("<");
    rightButtonLabel.begin({tft.width() * 0.55F, 0});
    rightButtonLabel.draw("X");
    scoreLabel.begin({tft.width(), 0});
    scoreKeeper.draw();
}

void loop()
{
    if (lastNavigationEvent != NavigationEvent::None)
    {
        const auto currentTime = millis();
        if (currentTime - lastNavigationEventTime > debounceDelay)
        {
            keyboard.handleNavigationEvent(lastNavigationEvent);
            lastNavigationEvent = NavigationEvent::None;
        }
    }
    if (lastButtonEvent != ButtonEvent::None)
    {
        const auto currentTime = millis();
        if (currentTime - lastButtonEventTime > debounceDelay)
        {
            mathsQuiz.handleButtonEvent(lastButtonEvent);
            lastButtonEvent = ButtonEvent::None;
        }
    }
    else if (digitalRead(WIO_KEY_A) == LOW)
    {
        const auto currentTime = millis();
        // Longer debounce needed for polling at LOW state (opposed to an edge)
        // because the button is held down "longer" than a single edge event.
        if (currentTime - lastButtonEventTime > debounceDelay * 5)
        {
            mathsQuiz.handleButtonEvent(ButtonEvent::Right);
            lastButtonEventTime = currentTime;
        }
    }
}
