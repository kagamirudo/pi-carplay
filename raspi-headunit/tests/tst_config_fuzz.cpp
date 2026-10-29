// Deterministic mutation fuzzing of the config parser. Whatever bytes are on
// the SD card, parsing must return (never crash, hang or overflow), reject
// with a message, and anything it accepts must survive a serialize round trip.
#include "ConfigFixtures.hpp"
#include "core/ConfigurationManager.hpp"

#include <QTest>

#include <random>
#include <string>
#include <vector>

using headunit::ConfigurationManager;
using headunit::test::kOddInputs;

namespace {

const std::vector<std::string>& kSeeds = headunit::test::kFuzzSeeds;

const char kAlphabet[] = "{}[]\":,\\-0123456789eE.truefalsnul \t\n\r\x00\x01\x7f\xc3\xa9\xff";

std::string mutate(std::string text, std::mt19937& rng) {
    std::uniform_int_distribution<int> op(0, 5);
    std::uniform_int_distribution<int> count(1, 8);
    std::uniform_int_distribution<int> alpha(0, sizeof(kAlphabet) - 2);
    const int edits = count(rng);
    for (int n = 0; n < edits; ++n) {
        std::uniform_int_distribution<std::size_t> pos(0, text.empty() ? 0 : text.size() - 1);
        const std::size_t at = pos(rng);
        switch (op(rng)) {
            case 0:
                if (!text.empty()) text[at] = kAlphabet[alpha(rng)];
                break;
            case 1:
                text.insert(text.begin() + static_cast<std::ptrdiff_t>(std::min(at, text.size())), kAlphabet[alpha(rng)]);
                break;
            case 2:
                if (!text.empty()) text.erase(at, 1);
                break;
            case 3:
                text.resize(std::min(text.size(), at));
                break;
            case 4:
                if (!text.empty()) text.insert(at, text.substr(at, count(rng) * 4));
                break;
            case 5:
                if (!text.empty()) text[at] = static_cast<char>(text[at] ^ (1 << (rng() % 8)));
                break;
        }
    }
    return text;
}

void checkOne(const std::string& input) {
    const auto parsed = ConfigurationManager::parse(input);
    if (!parsed.ok) {
        QVERIFY2(!parsed.error.empty(), input.c_str());
        return;
    }
    const auto again = ConfigurationManager::parse(ConfigurationManager::serialize(parsed.config));
    QVERIFY2(again.ok, again.error.c_str());
    QCOMPARE(ConfigurationManager::serialize(again.config), ConfigurationManager::serialize(parsed.config));
}

}  // namespace

class ConfigFuzzTest : public QObject {
    Q_OBJECT

private slots:
    void seedsParse() {
        for (const auto& seed : kSeeds) {
            const auto parsed = ConfigurationManager::parse(seed);
            QVERIFY2(parsed.ok, (seed + " -> " + parsed.error).c_str());
        }
    }

    void mutations() {
        const int iterations = qEnvironmentVariableIntValue("HEADUNIT_FUZZ_ITERATIONS") > 0
                                   ? qEnvironmentVariableIntValue("HEADUNIT_FUZZ_ITERATIONS")
                                   : 50000;
        std::mt19937 rng(0x5eed);
        for (int i = 0; i < iterations; ++i) {
            const std::string input = mutate(kSeeds[static_cast<std::size_t>(i) % kSeeds.size()], rng);
            checkOne(input);
            if (QTest::currentTestFailed()) {
                qWarning("failing input #%d", i);
                return;
            }
        }
    }

    void pathological_data() {
        QTest::addColumn<QByteArray>("input");
        QTest::newRow("nul bytes") << QByteArray("{\"a\":\"\0\0\"}", 10);
        QTest::newRow("only nul") << QByteArray(64, '\0');
        QTest::newRow("long string") << QByteArray("{\"bluetooth\":{\"carDevice\":\"" + QByteArray(1 << 20, 'a') + "\"}}");
        QTest::newRow("many keys") << [] {
            QByteArray text("{");
            for (int i = 0; i < 20000; ++i) text += "\"k" + QByteArray::number(i) + "\":1,";
            text += "\"z\":0}";
            return text;
        }();
        QTest::newRow("deep") << QByteArray(200000, '{');
        QTest::newRow("deep keys") << [] {
            QByteArray text;
            for (int i = 0; i < 50000; ++i) text += "{\"a\":";
            return text;
        }();
        for (const auto& [name, text] : kOddInputs) {
            QTest::newRow(name) << QByteArray(text);
        }
    }

    void pathological() {
        QFETCH(QByteArray, input);
        checkOne(input.toStdString());
    }
};

QTEST_GUILESS_MAIN(ConfigFuzzTest)
#include "tst_config_fuzz.moc"
