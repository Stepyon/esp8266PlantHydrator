#include <UniversalTelegramBot.h>
WiFiClientSecure secured_client;
UniversalTelegramBot* bot;
String chatId;
String token;

void initTGbot (const String& bot_token, const String& chat_Id) {
    if(bot == nullptr) {
        configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
        secured_client.setTrustAnchors(new X509List(TELEGRAM_CERTIFICATE_ROOT)); 
    }
    if(chatId != chat_Id || token != bot_token) {
        chatId = chat_Id;
        token = bot_token;
        bot = new UniversalTelegramBot(token, secured_client);
    }
}

bool sendTGMessage(const String& message) {
    if (bot != nullptr) {  // Check if the bot pointer is not null
        return bot->sendMessage(chatId, message, "");  // Use the arrow operator to call
    }
    return false;
}