#include <AsyncUDP.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceID3.h>
#include <M5Atom.h>
#include <string>
#include <WiFi.h>

#define SCK  23
#define MISO 33
#define MOSI 19


char buf; // direction command
char ssid[] = "M5StickC-Controller";
char pass[] = "controller";
AsyncUDP udp; // udp instance
unsigned int port = 8888; // local port to listen on

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

// Called when there's a warning or error (like a buffer underflow or decode hiccup).
void StatusCallback(void *cbData, int code, const char *string) {
    const char *ptr = reinterpret_cast<const char *>(cbData);
    // Note that the string may be in PROGMEM, so copy it to RAM for printf.
    char s1[64];
    strncpy_P(s1, string, sizeof(s1));
    s1[sizeof(s1) - 1] = 0;
    Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
    Serial.flush();
}


void MDCallback(
    void *cbData,
    const char *type,
    bool isUnicode,
    const char *string
) {
    (void)cbData;
    Serial.printf("ID3 callback for: %s = '", type);

    if (isUnicode) {
        string += 2;
    }

    while (*string) {
        char a = *(string++);
        if (isUnicode) {
            string++;
        }
        Serial.printf("%c", a);
    }
    Serial.printf("'\n");
    Serial.flush();
}


void playMP3(const char *filename) {
    Serial.printf("MP3 done\n");
    M5.dis.drawpix(0, 0x00ff00);
    audioLogger = &Serial;
    file        = new AudioFileSourceSD(filename);
    id3         = new AudioFileSourceID3(file);
    id3->RegisterMetadataCB(MDCallback, (void *)"ID3TAG");
    out = new AudioOutputI2S();
    out->SetPinout(22, 21, 25);
    mp3 = new AudioGeneratorMP3();
    mp3->RegisterStatusCB(StatusCallback, (void *)"mp3");
    mp3->begin(id3, out);
}


void readBuf() {
    char c = buf;
    if (c == 'a' || c == 'A' || c == 'c' || c == 'C' || c == 'd' || c == 'D') {
        playMP3("/forward.mp3");
    } else if (c == 'b' || c == 'B' || c == 'e' || c == 'E' || c == 'f' || c == 'F') {
        playMP3("/backward.mp3");
    } else if (c == 'g' || c == 'G' || c == 'h' || c == 'H') {
        playMP3("/turn.mp3");
    }
}


void setup() {
    Serial.begin(9600);
    M5.begin(true, false, true);
    SPI.begin(SCK, MISO, MOSI, -1);
    if (!SD.begin(-1, SPI, 40000000)) {
        Serial.println("Card Mount Failed");
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    delay(100);

    playMP3("/tamakon.mp3");

    if (udp.listen(port)) {
        udp.onPacket([](AsyncUDPPacket packet) {
            buf = (char)*(packet.data());
        });
    }
}


void loop() {
    M5.update();
    if (mp3->isRunning()) {
        if (!mp3->loop()) mp3->stop();
    } else if (M5.Btn.wasPressed()) {
        delay(500);
        playMP3("/tamakon.mp3");
    } else {
        delay(100);
        readBuf();
    }
}

