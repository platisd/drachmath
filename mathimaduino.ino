#include "TFT_eSPI.h"
#include "mathimaduino.hpp"

TFT_eSPI tft;

const TftColor screenBackgroundColor = colors::Red;
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
RectangleDimensions
    screenAreaExcludingButtonLabels{}; // Will be initialized in setup()
const auto clearScreenExcludingButtonLabels = []
{
    tft.fillRoundRect(screenAreaExcludingButtonLabels.x0,
                      screenAreaExcludingButtonLabels.y0,
                      screenAreaExcludingButtonLabels.width,
                      screenAreaExcludingButtonLabels.height,
                      screenAreaExcludingButtonLabels.radius,
                      makeColor(screenBackgroundColor));
};

const KeyColors menuColors
    = {colors::White, colors::Black, colors::Green, colors::Gray};
auto settingsEntries = makeSettingsEntries(
    SettingsEntry{MenuEntry{"Max operand value"},
                  makeMenuEntryConfig("10", "100")},
    SettingsEntry{MenuEntry{"Max result value"},
                  makeMenuEntryConfig("10", "100", "1000")},
    SettingsEntry{MenuEntry{"Math operations"},
                  makeMenuEntryConfig("+", "+-", "+-*", "+-*/")},
    SettingsEntry{MenuEntry{"Language"},
                  makeMenuEntryConfig("Greek", "English")},
    SettingsEntry{MenuEntry{"Max word length"},
                  makeMenuEntryConfig("5", "10", "15")});

auto settingsMenu = makeSettingsMenu(tft, settingsEntries, menuColors);

auto mainMenuEntries = makeMenuEntries(
    MenuEntry{
        "Maths Quiz",
        []
        {
            clearScreenExcludingButtonLabels();
            const RectangleDimensions keyboardAtBottom{0,
                                                       tft.height() * 2.0F
                                                           / 3.0F,
                                                       tft.width(),
                                                       tft.height() / 3.0F,
                                                       5};
            keyboard.begin(keyboardAtBottom);
            keyboard.draw();
            const RectangleDimensions mathsQuizAboveKeyboardInTheMiddle{
                0, tft.height() / 3.0F, tft.width(), tft.height() / 3.0F, 5};
            mathsQuiz.begin(mathsQuizAboveKeyboardInTheMiddle);
            mathsQuiz.drawNewQuestion();
            return false; // Disable the main menu since we are now in the quiz
        }},
    MenuEntry{"Spelling Quiz"},
    MenuEntry{
        "Settings",
        []
        {
            clearScreenExcludingButtonLabels();
            const RectangleDimensions settingsMenuLargeInTheMiddle{
                tft.width() * 0.025F,
                tft.height() / 5.0F,
                tft.width() * 0.95F,
                tft.height() * 0.5F,
                5};
            settingsMenu.begin(settingsMenuLargeInTheMiddle);
            settingsMenu.draw();
            return false; // Disable the main menu since we are now in settings
        }},
    MenuEntry{"About"});
auto mainMenu = makeMenu(tft, mainMenuEntries, menuColors);

auto navigationListeners = makeNavigationListeners(
    [](NavigationEvent event) { keyboard.handleNavigationEvent(event); },
    [](NavigationEvent event) { settingsMenu.handleNavigationEvent(event); },
    [](NavigationEvent event) { mainMenu.handleNavigationEvent(event); });
auto buttonListeners = makeButtonListeners(
    [](ButtonEvent event)
    {
        const auto shouldExit = mathsQuiz.handleButtonEvent(event);
        if (shouldExit)
        {
            keyboard.enableKeyboard(false);
            clearScreenExcludingButtonLabels();
            mainMenu.enableMenu(true);
            mainMenu.draw();
        }
    },
    [](ButtonEvent event)
    {
        const auto shouldExit = settingsMenu.handleButtonEvent(event);
        if (shouldExit)
        {
            clearScreenExcludingButtonLabels();
            mainMenu.enableMenu(true);
            mainMenu.draw();
        }
    },
    [](ButtonEvent event) { mainMenu.handleButtonEvent(event); });
auto inputHandler = makeInputHandler(navigationListeners, buttonListeners);

void attachInterrupts()
{
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_UP),
        [] { inputHandler.updateNavigationEvent(NavigationEvent::Up); },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_DOWN),
        [] { inputHandler.updateNavigationEvent(NavigationEvent::Down); },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_LEFT),
        [] { inputHandler.updateNavigationEvent(NavigationEvent::Left); },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_RIGHT),
        [] { inputHandler.updateNavigationEvent(NavigationEvent::Right); },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_5S_PRESS),
        [] { inputHandler.updateNavigationEvent(NavigationEvent::Press); },
        RISING);
    // Cannot attach an interrupt to both WIO_KEY_A and WIO_5S_UP
    // as they share the same external interrupt line. Poll instead.
    // attachInterrupt(
    //     digitalPinToInterrupt(WIO_KEY_A),
    //     []
    //     {
    //         inputHandler.updateButtonEvent(ButtonEvent::Right);
    //     },
    //     RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_KEY_B),
        [] { inputHandler.updateButtonEvent(ButtonEvent::Middle); },
        RISING);
    attachInterrupt(
        digitalPinToInterrupt(WIO_KEY_C),
        [] { inputHandler.updateButtonEvent(ButtonEvent::Left); },
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

    tft.fillScreen(makeColor(screenBackgroundColor));
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
    screenAreaExcludingButtonLabels
        = {0,
           20, // Roughly the height of the button labels
           tft.width(),
           tft.height() - 20, // Roughly the height of the button labels
           0};
    const RectangleDimensions menuInTheMiddle{tft.width() / 5.5F,
                                              tft.height() / 5.0F,
                                              tft.width() / 1.50F,
                                              tft.height() / 3.0F,
                                              5};
    mainMenu.begin(menuInTheMiddle);
    mainMenu.draw();
}

void loop()
{
    inputHandler.handleEvents();
}
