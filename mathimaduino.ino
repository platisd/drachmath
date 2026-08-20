#include <SPI.h>
#include <Seeed_FS.h>

#include "SD/Seeed_SD.h"

#include "TFT_eSPI.h"

#include "mathimaduino.hpp"

TFT_eSPI tft;

void warnNoSdCard(int x0,
                  int y0,
                  int textSize,
                  int alignment,
                  TftColor textColor,
                  TftColor backgroundColor)
{
    tft.setTextColor(makeColor(textColor), makeColor(backgroundColor));
    tft.setTextSize(textSize);
    tft.setTextDatum(alignment);
    tft.drawString("No SD card: Will lose settings on power off", x0, y0);
}

const TftColor screenBgColor = colors::Teal;
auto scoreLabel
    = makeLabel(tft, colors::Black, colors::White, screenBgColor, 3);
auto scoreKeeper        = makeScoreKeeper(scoreLabel, tft);
auto mathsQuizListeners = makeQuizListeners(
    [](bool correct)
    {
        if (correct)
        {
            scoreKeeper.increment();
        }
        scoreKeeper.logAnswer(QuizType::Maths, correct);
    });
SettingsHolder settingsHolder{};
auto mathsQuiz = makeMathsQuiz(
    tft, screenBgColor, colors::White, mathsQuizListeners, settingsHolder);
auto mathKeyboardListeners = makeKeyboardListeners(
    [](char key) { mathsQuiz.handleKeyboardPress(key); });
auto mathKeyboardLabels
    = makeKeyLabels('1', '2', '3', '4', '5', '6', '7', '8', '9', '0');
const KeyColors keyboardColors
    = {colors::White, colors::Black, colors::Navy, colors::Gray};
auto mathKeyboard = makeKeyboard<2, 5>(
    tft, keyboardColors, mathKeyboardLabels, mathKeyboardListeners);
auto leftButtonLabel
    = makeLabel(tft, colors::Black, colors::White, screenBgColor, 2);
auto middleButtonLabel
    = makeLabel(tft, colors::Black, colors::White, screenBgColor, 2);
auto rightButtonLabel
    = makeLabel(tft, colors::Black, colors::White, screenBgColor, 2);
RectangleDimensions
    screenAreaExcludingButtonLabels{}; // Will be initialized in setup()
const auto clearScreenExcludingButtonLabels = []
{
    tft.fillRoundRect(screenAreaExcludingButtonLabels.x0,
                      screenAreaExcludingButtonLabels.y0,
                      screenAreaExcludingButtonLabels.width,
                      screenAreaExcludingButtonLabels.height,
                      screenAreaExcludingButtonLabels.radius,
                      makeColor(screenBgColor));
};

auto greekSpellingQuizListeners = makeQuizListeners(
    [](bool correct)
    {
        if (correct)
        {
            scoreKeeper.increment();
        }
        scoreKeeper.logAnswer(QuizType::Spelling, correct);
    });

constexpr size_t greekWordBufferSize
    = maxGreekWordLetters * sizeof(uint16_t) + 1
      + 10; // 10 extra bytes for safety in case there's a long word in the
            // file. The +1 is for the null terminator.
auto greekWordsFileReader
    = makeFileReader<greekWordBufferSize>("greek_words.txt", SD);

auto greekSpellingQuiz = makeGreekSpellingQuiz(tft,
                                               greekSpellingQuizListeners,
                                               settingsHolder,
                                               greekWordsFileReader,
                                               screenBgColor,
                                               colors::White);

auto greekSpellingKeyboardListeners = makeKeyboardListeners(
    [](uint16_t greekChar)
    { greekSpellingQuiz.handleKeyboardPress(greekChar); });

auto epsilonKeyboardLabels = makeKeyLabels(greek::epsilon,
                                           greek::alpha,
                                           greek::iota,
                                           greek::epsilonTonos,
                                           greek::alphaTonos,
                                           greek::iotaTonos);
auto epsilonKeyboard       = makeKeyboard<2, 3>(
    tft, keyboardColors, epsilonKeyboardLabels, greekSpellingKeyboardListeners);

auto iotaKeyboardLabels = makeKeyLabels(greek::iota,
                                        greek::eta,
                                        greek::upsilon,
                                        greek::epsilon,
                                        greek::omicron,
                                        greek::iotaTonos,
                                        greek::etaTonos,
                                        greek::upsilonTonos);
auto iotaKeyboard       = makeKeyboard<2, 4>(
    tft, keyboardColors, iotaKeyboardLabels, greekSpellingKeyboardListeners);

auto omicronKeyboardLabels = makeKeyLabels(
    greek::omicron, greek::omega, greek::omicronTonos, greek::omegaTonos);
auto omicronKeyboard = makeKeyboard<2, 2>(
    tft, keyboardColors, omicronKeyboardLabels, greekSpellingKeyboardListeners);

auto avKeyboardLabels = makeKeyLabels(greek::alpha,
                                      greek::upsilon,
                                      greek::beta,
                                      greek::phi,
                                      greek::upsilonTonos,
                                      greek::alphaTonos);
auto avKeyboard       = makeKeyboard<2, 3>(
    tft, keyboardColors, avKeyboardLabels, greekSpellingKeyboardListeners);

auto evKeyboardLabels = makeKeyLabels(greek::epsilon,
                                      greek::upsilon,
                                      greek::beta,
                                      greek::phi,
                                      greek::upsilonTonos,
                                      greek::epsilonTonos);
auto evKeyboard       = makeKeyboard<2, 3>(
    tft, keyboardColors, evKeyboardLabels, greekSpellingKeyboardListeners);

auto gammaKeyboardLabels = makeKeyLabels(greek::gamma, greek::kappa);
auto gammaKeyboard       = makeKeyboard<1, 2>(
    tft, keyboardColors, gammaKeyboardLabels, greekSpellingKeyboardListeners);

RectangleDimensions keyboardAtBottom{};
RectangleDimensions quizAboveKeyboardInTheMiddle{};

const auto drawGreekKeyboard = [](GreekHomophoneGroup group)
{
    const auto beginAndDrawKeyboard = [](auto& keyboard)
    {
        keyboard.begin(keyboardAtBottom);
        keyboard.draw();
    };
    // Disable all keyboards first to avoid any overlap
    epsilonKeyboard.enableKeyboard(false);
    iotaKeyboard.enableKeyboard(false);
    omicronKeyboard.enableKeyboard(false);
    avKeyboard.enableKeyboard(false);
    evKeyboard.enableKeyboard(false);
    gammaKeyboard.enableKeyboard(false);
    switch (group)
    {
    case GreekHomophoneGroup::EpsilonSound:
        return beginAndDrawKeyboard(epsilonKeyboard);
    case GreekHomophoneGroup::IotaSound:
        return beginAndDrawKeyboard(iotaKeyboard);
    case GreekHomophoneGroup::OmicronSound:
        return beginAndDrawKeyboard(omicronKeyboard);
    case GreekHomophoneGroup::AvSound:
        return beginAndDrawKeyboard(avKeyboard);
    case GreekHomophoneGroup::EvSound:
        return beginAndDrawKeyboard(evKeyboard);
    case GreekHomophoneGroup::GammaNasal:
        return beginAndDrawKeyboard(gammaKeyboard);
    default:
        return beginAndDrawKeyboard(iotaKeyboard);
    }
};

const KeyColors menuColors
    = {colors::White, colors::Black, colors::Navy, colors::Gray};
auto settingsEntries = makeSettingsEntries(
    SettingsEntry{MenuEntry{"Max operand value"},
                  makeMenuEntryConfig("10", "100"),
                  "max_operand.txt",
                  [](auto v) { settingsHolder.setMaxOperand(v); }},
    SettingsEntry{MenuEntry{"Max result value"},
                  makeMenuEntryConfig("10", "100", "1000", "10000"),
                  "max_result.txt",
                  [](auto v) { settingsHolder.setMaxResult(v); }},
    SettingsEntry{MenuEntry{"Math operations"},
                  makeMenuEntryConfig("+", "+-", "+-*", "+-*/"),
                  "operations_count.txt",
                  [](auto v) { settingsHolder.setOperationsCount(v); }},
    SettingsEntry{MenuEntry{"Language"},
                  makeMenuEntryConfig("Greek"),
                  "language.txt",
                  [](auto v) { settingsHolder.setLanguage(v); }},
    SettingsEntry{MenuEntry{"Max word length"},
                  makeMenuEntryConfig("5", "10", "15"),
                  "max_word_length.txt",
                  [](auto v) { settingsHolder.setMaxWordLength(v); }},
    SettingsEntry{MenuEntry{"Sound"},
                  makeMenuEntryConfig("On", "Off"),
                  "sound.txt",
                  [](auto v) { settingsHolder.setSound(v); }});

auto settingsMenu = makeSettingsMenu(tft, settingsEntries, menuColors);
auto persistentSettings
    = makePersistentSettings(settingsHolder, SD, settingsEntries);
auto sdCardChecker = makeSdCardChecker(
    makeFileWriter("sd_card_test.txt", SD),
    [] { return SD.begin(SDCARD_SS_PIN, SDCARD_SPI); },
    [] { return digitalRead(SDCARD_DET_PIN) == LOW; },
    []
    {
        persistentSettings.load();
        tft.unloadFont(); // Just in case
        tft.loadFont("ubuntu-greek-latin-32");
        ScopedGreekFont greekFontQuickUnloader{tft};
        auto lockFileReader = makeFileReader<16>("lock_settings.txt", SD);
        auto line           = lockFileReader.readLine();
        settingsMenu.lockMenu(line && strcmp(line, "1") == 0);
    },
    [] { SD.end(); });

auto statsScreen = makeStatsScreen(
    tft, scoreKeeper, settingsHolder, colors::Black, colors::White);

auto batteryIndicator
    = makeBatteryIndicator(tft, [] { return 10; }, screenBgColor);

auto mainMenuEntries = makeMenuEntries(
    MenuEntry{
        "Maths Quiz",
        []
        {
            clearScreenExcludingButtonLabels();
            middleButtonLabel.draw("Del");
            rightButtonLabel.draw("Esc");
            mathKeyboard.begin(keyboardAtBottom);
            mathKeyboard.draw();
            mathsQuiz.begin(quizAboveKeyboardInTheMiddle);
            mathsQuiz.drawNewQuestion();
            return false; // Disable the main menu since we are now in the quiz
        }},
    MenuEntry{
        "Spelling Quiz",
        []
        {
            clearScreenExcludingButtonLabels();
            middleButtonLabel.draw("Del");
            rightButtonLabel.draw("Esc");
            greekSpellingQuiz.begin(quizAboveKeyboardInTheMiddle);
            greekSpellingQuiz.drawNewQuestion();
            return false; // Disable the main menu since we are now in the quiz
        }},
    MenuEntry{
        "Settings",
        []
        {
            clearScreenExcludingButtonLabels();
            const RectangleDimensions settingsMenuLargeInTheMiddle{
                tft.width() * 0.025F,
                tft.height() * 0.33F,
                tft.width() * 0.95F,
                tft.height() * 0.5F,
                5};
            settingsMenu.begin(settingsMenuLargeInTheMiddle);
            settingsMenu.draw();
            leftButtonLabel.clear(); // Nothing to select with button
            middleButtonLabel.clear();
            rightButtonLabel.draw("Esc");
            if (!sdCardChecker.isSdCardReadyToUse())
            {
                warnNoSdCard(tft.width() / 2,
                             tft.height(),
                             1,
                             TC_DATUM,
                             colors::Black,
                             screenBgColor);
            }
            return false; // Disable the main menu since we are now in settings
        }},
    MenuEntry{
        "Stats",
        []
        {
            clearScreenExcludingButtonLabels();
            const RectangleDimensions statsScreenLargeInTheMiddle{
                tft.width() * 0.025F,
                tft.height() * 0.25F,
                tft.width() * 0.95F,
                tft.height() * 0.58F,
                5};
            statsScreen.begin(statsScreenLargeInTheMiddle);
            leftButtonLabel.clear();
            middleButtonLabel.clear();
            scoreKeeper.hide(); // We already show the score in stats
            batteryIndicator.hide();
            rightButtonLabel.draw("Esc");
            statsScreen.draw();
            return false; // Disable the main menu since we are now in stats
        }});
auto mainMenu = makeMenu(tft, mainMenuEntries, menuColors);

auto buttonListeners = makeButtonListeners(
    [](ButtonEvent event)
    {
        const auto consumedEvent = mathsQuiz.isEnabled();
        const auto shouldExit    = mathsQuiz.handleButtonEvent(event);
        if (shouldExit)
        {
            mathKeyboard.enableKeyboard(false);
            mathsQuiz.enableQuiz(false);
            clearScreenExcludingButtonLabels();
            mainMenu.enableMenu(true);
            mainMenu.draw();
            leftButtonLabel.draw("Sel");
            middleButtonLabel.clear(); // Button not used in main menu
            rightButtonLabel.clear();
        }
        return consumedEvent; // Event consumed if quiz was initially enabled
    },
    [](ButtonEvent event)
    {
        const auto consumedEvent = settingsMenu.isEnabled();
        const auto shouldExit    = settingsMenu.handleButtonEvent(event);
        if (shouldExit)
        {
            if (sdCardChecker.isSdCardReadyToUse())
            {
                persistentSettings.save();
            }
            settingsMenu.enableMenu(false);
            clearScreenExcludingButtonLabels();
            mainMenu.enableMenu(true);
            mainMenu.draw();
            leftButtonLabel.draw("Sel");
            middleButtonLabel.clear();
            rightButtonLabel.clear();
        }
        return consumedEvent;
    },
    [](ButtonEvent event)
    {
        const auto consumedEvent = mainMenu.isEnabled();
        mainMenu.handleButtonEvent(event);
        return consumedEvent;
    },
    [](ButtonEvent event)
    {
        const auto consumedEvent = greekSpellingQuiz.isEnabled();
        const auto shouldExit    = greekSpellingQuiz.handleButtonEvent(event);
        if (shouldExit)
        {
            greekSpellingQuiz.enableQuiz(false);
            // Disable all keyboards just in case one of them is enabled
            epsilonKeyboard.enableKeyboard(false);
            iotaKeyboard.enableKeyboard(false);
            omicronKeyboard.enableKeyboard(false);
            avKeyboard.enableKeyboard(false);
            evKeyboard.enableKeyboard(false);
            gammaKeyboard.enableKeyboard(false);
            clearScreenExcludingButtonLabels();
            mainMenu.enableMenu(true);
            mainMenu.draw();
            leftButtonLabel.draw("Sel");
            middleButtonLabel.clear();
            rightButtonLabel.clear();
        }
        return consumedEvent;
    },
    [](ButtonEvent event)
    {
        const auto consumedEvent = statsScreen.isEnabled();
        const auto shouldExit
            = event == ButtonEvent::Right && statsScreen.isEnabled();
        if (shouldExit)
        {
            statsScreen.enableStatsScreen(false);
            clearScreenExcludingButtonLabels();
            mainMenu.enableMenu(true);
            mainMenu.draw();
            leftButtonLabel.draw("Sel");
            scoreKeeper.draw();
            batteryIndicator.draw();
            middleButtonLabel.clear();
            rightButtonLabel.clear();
        }
        return consumedEvent;
    });

auto navigationListeners = makeNavigationListeners(
    [](NavigationEvent event)
    {
        const auto consumedEvent = mathKeyboard.isEnabled();
        mathKeyboard.handleNavigationEvent(event);
        return consumedEvent;
    },
    [](NavigationEvent event)
    {
        const auto consumedEvent = settingsMenu.isEnabled();
        settingsMenu.handleNavigationEvent(event);
        return consumedEvent;
    },
    [](NavigationEvent event)
    {
        const auto consumedEvent = mainMenu.isEnabled();
        mainMenu.handleNavigationEvent(event);
        return consumedEvent;
    },
    [](NavigationEvent event)
    {
        const auto consumedEvent = greekSpellingQuiz.isEnabled();
        // Let's group all the Greek keyboards together for clarity
        // We rely on the irrelevant keyboards being disabled
        // and pass the event to all of them.
        // Only the enabled/correct keyboard should  respond to the event.
        epsilonKeyboard.handleNavigationEvent(event);
        iotaKeyboard.handleNavigationEvent(event);
        omicronKeyboard.handleNavigationEvent(event);
        avKeyboard.handleNavigationEvent(event);
        evKeyboard.handleNavigationEvent(event);
        gammaKeyboard.handleNavigationEvent(event);
        return consumedEvent;
    });
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
    // WIO_KEY_A and WIO_5S_UP interrupt clash
    // WIO_KEY_B and SDCARD_DET_PIN interrupt clash
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
    pinMode(SDCARD_DET_PIN, INPUT_PULLUP);
    pinMode(WIO_BUZZER, OUTPUT);
    attachInterrupts();
    Serial.begin(115200);
    sdCardChecker.begin();

    tft.begin();
    tft.setRotation(3);
    tft.setTextSize(2); // Explicit since it affects font height and width

    tft.fillScreen(makeColor(screenBgColor));
    // the button labels are on the top left of the screen and are a few pixels
    // large. They are roughly 10% of the screen away from each o ther
    leftButtonLabel.begin({tft.width() * 0.045F, 0});
    leftButtonLabel.draw("Sel");
    middleButtonLabel.begin({tft.width() * 0.28F, 0});
    rightButtonLabel.begin({tft.width() * 0.54F, 0});
    scoreLabel.begin({tft.width() - tft.textWidth("xx"), 0});
    scoreKeeper.draw();
    const RectangleDimensions menuInTheMiddle{tft.width() * 0.2F,
                                              tft.height() * 0.33F,
                                              tft.width() * 0.6F,
                                              tft.height() * 0.33F,
                                              5};
    mainMenu.begin(menuInTheMiddle);
    mainMenu.draw();
    greekSpellingQuiz.registerKeyboardDrawer(drawGreekKeyboard);
    keyboardAtBottom = RectangleDimensions{
        0, tft.height() * 2.0F / 3.0F, tft.width(), tft.height() / 3.0F, 5};
    quizAboveKeyboardInTheMiddle = RectangleDimensions{
        0, tft.height() / 3.0F, tft.width(), tft.height() / 3.0F, 5};

    tft.setTextSize(2);
    const RectangleDimensions batteryIndicatorUnderScoreLabel{
        tft.width() - tft.textWidth("XXXX"),
        tft.fontHeight(defaultEngFont) * 1.7F,
        tft.textWidth("XXXX"),
        tft.fontHeight(defaultEngFont),
        0};
    batteryIndicator.begin(batteryIndicatorUnderScoreLabel);
    const auto bottomOfBatteryIndicator
        = batteryIndicatorUnderScoreLabel.y0
          + batteryIndicatorUnderScoreLabel.height;
    screenAreaExcludingButtonLabels
        = {0,
           bottomOfBatteryIndicator, // The lowest button
           tft.width(),
           tft.height() - bottomOfBatteryIndicator,
           0};
}

void loop()
{
    inputHandler.handleEvents();
    sdCardChecker.handleSdCardActivity();
    batteryIndicator.update();
}
