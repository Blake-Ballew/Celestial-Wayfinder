    #pragma once

#include "globalDefines.h"
#include <ArduinoJson.h>
// #include "LED_Manager.h"
#include "NavigationUtils.h"
#include <string>
#include <vector>
#include <cstring>

// ---------------------------------------------------------------------------
// Wire-format keys  (single-char for compact MsgPack, matches legacy format)
// ---------------------------------------------------------------------------
namespace MsgKey
{
    static const char *const SCHEMA_VERSION PROGMEM = "t"; // was "message type" in v0
    static const char *const ID             PROGMEM = "i";
    static const char *const BOUNCES_LEFT   PROGMEM = "B";
    static const char *const RECIPIENT      PROGMEM = "R";
    static const char *const FROM           PROGMEM = "f";
    static const char *const FROM_NAME      PROGMEM = "n";
    static const char *const TIME           PROGMEM = "T";
    static const char *const DATE           PROGMEM = "D";
    static const char *const COLOR_R        PROGMEM = "r";
    static const char *const COLOR_G        PROGMEM = "g";
    static const char *const COLOR_B        PROGMEM = "b";
    static const char *const LAT            PROGMEM = "a";
    static const char *const LNG            PROGMEM = "o";
    static const char *const STATUS         PROGMEM = "s";
    static const char *const IS_LIVE        PROGMEM = "L";
}

// ---------------------------------------------------------------------------
// Printable information helper (unchanged from legacy)
// ---------------------------------------------------------------------------
struct MessagePrintInformation
{
    static constexpr size_t MAX_LEN = 64;
    char txt[MAX_LEN];

    MessagePrintInformation(const char *src)
    {
        strncpy(txt, src, MAX_LEN - 1);
        txt[MAX_LEN - 1] = '\0';
    }

    MessagePrintInformation(const MessagePrintInformation &other)
    {
        memcpy(txt, other.txt, MAX_LEN);
    }
};

// ---------------------------------------------------------------------------
// Message  –  flat, standalone message used over LoRa
//
// Schema version 0 is wire-compatible with the legacy MessagePing format.
// Old firmware reads the "t" key as message type (was 0 for Ping) and will
// parse everything else normally.
// ---------------------------------------------------------------------------
class Message
{
public:
    // ----- constants -------------------------------------------------------
    static constexpr size_t   MAX_NAME_LENGTH   = 12;
    static constexpr size_t   MAX_STATUS_LENGTH = 23;
    static constexpr size_t   BUFFER_SIZE       = 512;
    static constexpr uint8_t  CURRENT_SCHEMA    = 0;   // matches legacy MessagePing type
    static constexpr uint8_t  DEFAULT_BOUNCES   = 5;

    // ----- fields (all public) ---------------------------------------------

    // Schema / wire version.  Occupies the legacy "message type" key on the wire.
    uint8_t  schemaVersion = CURRENT_SCHEMA;

    // Unique message ID (random if not supplied)
    uint32_t msgID = 0;

    // Hop counter – each relay decrements
    uint8_t  bouncesLeft = DEFAULT_BOUNCES;

    // Addressing
    uint32_t recipient = 0;   // 0 = broadcast
    uint32_t sender    = 0;

    // Sender display name (truncated to MAX_NAME_LENGTH)
    std::string senderName;

    // Timestamps
    uint32_t time = 0;
    uint32_t date = 0;

    // Colour
    uint8_t color_R = 0;
    uint8_t color_G = 0;
    uint8_t color_B = 0;

    // Position
    double lat = 0.0;
    double lng = 0.0;

    // Status text (truncated to MAX_STATUS_LENGTH)
    std::string status;

    // Live-location flag
    bool isLive = false;

    // ----- constructors ----------------------------------------------------

    Message() = default;

    Message(uint32_t time,
            uint32_t date,
            uint32_t recipient,
            uint32_t sender,
            const char *senderName,
            uint32_t msgID,
            uint8_t color_R,
            uint8_t color_G,
            uint8_t color_B,
            double lat,
            double lng,
            const char *status)
        : time(time)
        , date(date)
        , recipient(recipient)
        , sender(sender)
        , senderName(senderName ? std::string(senderName, strnlen(senderName, MAX_NAME_LENGTH)) : "")
        , msgID(msgID != 0 ? msgID : esp_random())
        , bouncesLeft(DEFAULT_BOUNCES)
        , color_R(color_R)
        , color_G(color_G)
        , color_B(color_B)
        , lat(lat)
        , lng(lng)
        , status(status ? std::string(status, strnlen(status, MAX_STATUS_LENGTH)) : "")
        , isLive(false)
    {
    }

    ~Message() = default;

    // ----- serialization ---------------------------------------------------

    bool serialize(JsonDocument &doc) const
    {
        doc[MsgKey::SCHEMA_VERSION] = schemaVersion;
        doc[MsgKey::ID]             = msgID;
        doc[MsgKey::BOUNCES_LEFT]   = bouncesLeft;
        doc[MsgKey::RECIPIENT]      = recipient;
        doc[MsgKey::FROM]           = sender;
        doc[MsgKey::FROM_NAME]      = senderName;
        doc[MsgKey::TIME]           = time;
        doc[MsgKey::DATE]           = date;
        doc[MsgKey::COLOR_R]        = color_R;
        doc[MsgKey::COLOR_G]        = color_G;
        doc[MsgKey::COLOR_B]        = color_B;
        doc[MsgKey::LAT]            = lat;
        doc[MsgKey::LNG]            = lng;
        doc[MsgKey::STATUS]         = status;
        doc[MsgKey::IS_LIVE]        = isLive;

        return !doc.overflowed();
    }

    void deserialize(const JsonDocument &doc)
    {
        // Schema version (legacy "message type")
        schemaVersion = readOr<uint8_t>(doc, MsgKey::SCHEMA_VERSION, CURRENT_SCHEMA);

        msgID        = readOr<uint32_t>(doc, MsgKey::ID,           0);
        bouncesLeft  = readOr<uint8_t> (doc, MsgKey::BOUNCES_LEFT, 0);
        recipient    = readOr<uint32_t>(doc, MsgKey::RECIPIENT,    0);
        sender       = readOr<uint32_t>(doc, MsgKey::FROM,         0);
        time         = readOr<uint32_t>(doc, MsgKey::TIME,         0);
        date         = readOr<uint32_t>(doc, MsgKey::DATE,         0);

        color_R      = readOr<uint8_t>(doc, MsgKey::COLOR_R, 0);
        color_G      = readOr<uint8_t>(doc, MsgKey::COLOR_G, 0);
        color_B      = readOr<uint8_t>(doc, MsgKey::COLOR_B, 0);

        lat          = readOr<double>(doc, MsgKey::LAT, 0.0);
        lng          = readOr<double>(doc, MsgKey::LNG, 0.0);

        isLive       = readOr<bool>(doc, MsgKey::IS_LIVE, false);

        // Strings – read with length enforcement
        readString(doc, MsgKey::FROM_NAME, senderName, MAX_NAME_LENGTH);
        readString(doc, MsgKey::STATUS,    status,     MAX_STATUS_LENGTH);
    }

    // ----- factory ---------------------------------------------------------

    static Message *MessageFactory(const uint8_t *buffer, size_t len)
    {
        StaticJsonDocument<BUFFER_SIZE> doc;

        if (deserializeMsgPack(doc, reinterpret_cast<const char *>(buffer), len)
                != DeserializationError::Ok)
        {
            ESP_LOGW(TAG, "Message::MessageFactory: MsgPack deserialize failed");
            return nullptr;
        }

        Message *msg = new Message();
        msg->deserialize(doc);

        if (!msg->IsValid())
        {
            ESP_LOGW(TAG, "Message::MessageFactory: Invalid message");
            delete msg;
            return nullptr;
        }

        return msg;
    }

    // ----- utilities -------------------------------------------------------

    Message *clone() const
    {
        Message *copy  = new Message(time, date, recipient, sender,
                                     senderName.c_str(), msgID,
                                     color_R, color_G, color_B,
                                     lat, lng, status.c_str());
        copy->bouncesLeft   = bouncesLeft;
        copy->isLive        = isLive;
        copy->schemaVersion = schemaVersion;
        return copy;
    }

    bool IsValid() const
    {
        return (msgID != 0 && sender != 0);
    }

    void GetPrintableInformation(std::vector<MessagePrintInformation> &info) const
    {
        info.emplace_back(senderName.c_str());
        info.emplace_back(status.c_str());
    }

    static void printFromBuffer(const uint8_t *buffer, size_t len = BUFFER_SIZE)
    {
        StaticJsonDocument<BUFFER_SIZE> doc;
        deserializeMsgPack(doc, reinterpret_cast<const char *>(buffer), len);
        serializeJson(doc, Serial);
    }

    /// Returns a human-readable age string from a packed time-difference value.
    static std::string GetMessageAge(uint64_t timeDiff)
    {
        uint8_t diffHours   = (timeDiff & 0xFF000000) >> 24;
        uint8_t diffMinutes = (timeDiff & 0xFF0000)   >> 16;

        if (timeDiff > 0xFFFFFFFF)  return ">1d";
        if (diffHours   > 0)        return std::to_string(diffHours)   + "h";
        if (diffMinutes > 0)        return std::to_string(diffMinutes) + "m";
        return "<1m";
    }

    /// Read schema version from raw MsgPack without full deserialization.
    static uint8_t GetSchemaVersionFromBuffer(const uint8_t *buffer, size_t len = BUFFER_SIZE)
    {
        if (!buffer) return 0;
        StaticJsonDocument<BUFFER_SIZE> doc;
        if (deserializeMsgPack(doc, buffer, len) != DeserializationError::Ok) return 0;
        return doc.containsKey(MsgKey::SCHEMA_VERSION)
                   ? doc[MsgKey::SCHEMA_VERSION].as<uint8_t>()
                   : 0;
    }

private:
    // ----- helpers ---------------------------------------------------------

    template <typename T>
    static T readOr(const JsonDocument &doc, const char *key, T fallback)
    {
        if (doc.containsKey(key))
            return doc[key].as<T>();

        ESP_LOGW(TAG, "Message::deserialize: key '%s' missing, using default", key);
        return fallback;
    }

    static void readString(const JsonDocument &doc, const char *key,
                           std::string &dest, size_t maxLen)
    {
        if (doc.containsKey(key))
        {
            const char *raw = doc[key].as<const char *>();
            if (raw)
            {
                dest.assign(raw, strnlen(raw, maxLen));
                return;
            }
        }
        dest.clear();
    }
};