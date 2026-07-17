#include "TFT_eSPI.h"
#include "mathimaduino.hpp"

TFT_eSPI tft;

volatile NavigationEvent lastEvent    = NavigationEvent::None;
volatile unsigned long lastEventTime  = 0;
constexpr unsigned long debounceDelay = 50; // milliseconds
const KeyColors keyColors
    = {colors::White, colors::Red, colors::Green, colors::Gray};
auto keyboard = makeKeyboard<2, 5>(
    tft, keyColors, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0');
auto mathsQuiz = makeMathsQuiz(tft, colors::Black, colors::White);

void attachNavigationInterrupts()
{
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_UP),
        []
        {
            lastEvent     = NavigationEvent::Up;
            lastEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_DOWN),
        []
        {
            lastEvent     = NavigationEvent::Down;
            lastEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_LEFT),
        []
        {
            lastEvent     = NavigationEvent::Left;
            lastEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_RIGHT),
        []
        {
            lastEvent     = NavigationEvent::Right;
            lastEventTime = millis();
        },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_PRESS),
        []
        {
            lastEvent     = NavigationEvent::Press;
            lastEventTime = millis();
        },
        RISING);
}

void setup()
{
    pinMode(WIO_5S_UP, INPUT_PULLUP);
    pinMode(WIO_5S_DOWN, INPUT_PULLUP);
    pinMode(WIO_5S_LEFT, INPUT_PULLUP);
    pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
    pinMode(WIO_5S_PRESS, INPUT_PULLUP);
    attachNavigationInterrupts();

    tft.begin();
    tft.setRotation(3);

    tft.fillScreen(TFT_RED);
    const RectangleDimensions keyboardAtBottom{
        0, tft.height() * 2 / 3, tft.width(), tft.height() / 3, 5};
    keyboard.begin(keyboardAtBottom);
    keyboard.draw();
    const RectangleDimensions mathsQuizAboveKeyboardInTheMiddle{
        0, tft.height() / 3, tft.width(), tft.height() / 3, 5};
    mathsQuiz.begin(mathsQuizAboveKeyboardInTheMiddle);
    mathsQuiz.draw();
}

void loop()
{
    if (lastEvent != NavigationEvent::None)
    {
        const auto currentTime = millis();
        if (currentTime - lastEventTime > debounceDelay)
        {
            keyboard.handleNavigationEvent(lastEvent);
            lastEvent = NavigationEvent::None;
        }
    }
}
