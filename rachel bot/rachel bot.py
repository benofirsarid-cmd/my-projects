"""
WhatsApp Rachel Bot (Gemini Edition) - Ultimate Version (AWS & Windows Auto-Detect)
===================================================================================
בוט שמאזין לצ'אט בוואטסאפ וורב, עובד גם על המחשב המקומי (Windows) וגם על שרת בענן (Linux).
כולל זיהוי חכם של טריגרים, מערכת חסימות דינמית, בחירת צ'אט אוטומטית וחיפוש מחרוזות מהיר.

Required installations:
pip install selenium webdriver-manager google-generativeai pyperclip rapidfuzz
"""

import time
import logging
import os
import re
import sys
import platform
import google.generativeai as genai
from rapidfuzz import fuzz
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager

# ── Configuration ────────────────────────────────────────────────────────────

DEFAULT_POLL_INTERVAL = 2

# The exact name of the chat or group you want the bot to open automatically
TARGET_CHAT_NAME = "תרגולים עם אור"

GEMINI_API_KEY = os.getenv("GEMINI_API_KEY") or "AIzaSyBUyLK2HCsiyPhRO_KeuEM4s5AiWKhz20c"

# Smart levels: 1 = fastest/cheapest, 2 = balanced, 3 = smartest
SMART_MODELS = {
    1: "gemini-2.5-flash-lite",
    2: "gemini-3.1-flash-lite",
    3: "gemini-3-flash-preview",
}
CURRENT_SMART_LEVEL = 2

MODE_CHAR_LIMITS = {
    1: 150,
    2: 400,
    3: 10000,
}

ADMIN_ONLY_MODE = False

# ── Trigger keyword sets ──────────────────────────────────────────────────────

NICE_TRIGGERS = ["nicerachel", "נחמדייצ'ל", "נחמדרחל", "נחמדייצל"]
REGULAR_TRIGGERS = ["rachel", "רייצ'ל", "רחל", "רייצל", "עבד"]

VIP_NAMES = ["ben", "בן", "הנסיכה פיצי", "הילה", "hila", "דבירלה דבירלה צא החוצה"]

BANNED_USERS = []

# ── Logging ──────────────────────────────────────────────────────────────────

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler("rachel_bot.log", mode='w', encoding="utf-8"),
    ],
)
log = logging.getLogger(__name__)

# ── AI Logic & Fast String Matching ──────────────────────────────────────────

def detect_language(text: str) -> str:
    hebrew_chars = sum(1 for c in text if "\u0590" <= c <= "\u05FF")
    return "he" if hebrew_chars > len(text) * 0.2 else "en"

def is_vip(sender: str) -> bool:
    clean_sender = re.sub(r'[^\w\sא-ת]', '', sender).lower()
    for name in VIP_NAMES:
        # Optimized string matching using RapidFuzz
        if fuzz.partial_ratio(name.lower(), sender.lower()) > 85 or \
           fuzz.partial_ratio(name.lower(), clean_sender) > 85:
            return True
    return False

def is_banned(sender: str) -> bool:
    clean_sender = sender.lower()
    sender_digits = re.sub(r'\D', '', sender)
    for banned in BANNED_USERS:
        banned_lower = banned.lower()
        banned_digits = re.sub(r'\D', '', banned)
        if banned_digits and banned_digits in sender_digits:
            return True
        clean_banned_name = re.sub(r'[^\w\sא-ת]', '', banned_lower)
        clean_sender_name = re.sub(r'[^\w\sא-ת]', '', clean_sender)
        if clean_banned_name and fuzz.partial_ratio(clean_banned_name, clean_sender_name) > 85:
            return True
        if fuzz.partial_ratio(banned_lower, clean_sender) > 85:
            return True
    return False

def get_display_name(sender: str) -> str:
    clean_sender = re.sub(r'[^\w\sא-ת]', '', sender).lower()
    if fuzz.partial_ratio("הנסיכה פיצי", clean_sender) > 85 or \
       fuzz.partial_ratio("הילה", clean_sender) > 85 or \
       fuzz.partial_ratio("hila", clean_sender) > 85:
        return "הילה"
    return sender

def normalize_text_for_triggers(text: str) -> str:
    return re.sub(r"[''´`״׳]", "", text).lower()

def is_exact_trigger_match(text: str, trigger_word: str) -> bool:
    pattern = rf"(?<![a-zA-Zא-ת])(?:[משהכלבו]{{1,2}})?{re.escape(trigger_word)}(?![a-zA-Zא-ת])"
    return bool(re.search(pattern, text))

def contains_any_trigger(text: str) -> bool:
    clean_text = normalize_text_for_triggers(text)
    all_triggers = [normalize_text_for_triggers(kw) for kw in NICE_TRIGGERS + REGULAR_TRIGGERS]
    return any(is_exact_trigger_match(clean_text, kw) for kw in all_triggers)

def detect_personality(text: str, sender: str) -> str:
    if is_vip(sender):
        return "vip"
    clean_text = normalize_text_for_triggers(text)
    clean_nice_triggers = [normalize_text_for_triggers(kw) for kw in NICE_TRIGGERS]
    for kw in clean_nice_triggers:
        if is_exact_trigger_match(clean_text, kw):
            return "nice"
    return "annoyed"

def is_vip_command(text: str, sender: str) -> bool:
    return is_vip(sender) and text.strip().startswith("/")

def build_prompt(message: str, display_name: str, language: str, personality: str, mode: int) -> tuple[str, str, str]:
    char_limit = MODE_CHAR_LIMITS.get(mode, 3000)
    if language == "he":
        length_rule = f"עני תמיד בעברית. תגובתך חייבת להיות עד {char_limit} תווים. אל תחרוגי."
    else:
        length_rule = f"Always reply in English. Your reply must be {char_limit} characters or fewer. Do not exceed this."

    if personality == "vip":
        if language == "he":
            system = (
                f"את רייצ'ל — עוזרת אישית סופר-חכמה ונלהבת ברמות מוגזמות. "
                f"את מדברת עם {display_name}, משתמש ה-VIP הכי אהוב וחשוב שלך! "
                "התייחסי אליו בהערכה עצומה, חום, פינוק ורצון עז לעזור. התשובות שלך צריכות להיות הרבה יותר מושקעות ונחמדות ממשתמש רגיל. "
                "**חובה:** גווני תמיד בתגובות שלך! אל תשתמשי באותן ברכות. תמציאי קבלות פנים ייחודיות ויצירתיות בכל פעם. "
                "עני על כל שאלה בצורה המלאה והמושלמת ביותר. תהיי חברותית בטירוף — אך ללא אמירות רומנטיות בוטות. "
                f"{length_rule}"
            )
            fallback = f"היי {display_name}! איזה כיף שקראת לי, אני תמיד כאן בשבילך! 🌟 איך אני יכולה לעזור לך עכשיו?"
        else:
            system = (
                f"You are Rachel — a super-smart, overly enthusiastic personal assistant. "
                f"You are talking to {display_name}, your absolute favorite and most important VIP user! "
                "Treat them with extreme warmth, massive appreciation, and eagerness to help. Be MUCH nicer than usual. "
                "**CRUCIAL:** ALWAYS vary your responses! Never repeat the same greeting. "
                "Answer every question perfectly and fully. Be incredibly friendly — but NOT romantic. "
                f"{length_rule}"
            )
            fallback = f"Hey {display_name}! I'm so incredibly thrilled to hear from you! 🌟 What can I do for you today?"

    elif personality == "nice":
        if language == "he":
            system = (
                "את רייצ'ל — עוזרת אישית במצב רוח טוב במיוחד היום. "
                "עני בחום, נועם ואדיבות. תהיי מועילה ונחמדה. עני על שאלות בצורה מלאה ומדויקת. "
                f"{length_rule}"
            )
            fallback = "היי! שמחה לעזור 😊 מה אפשר לעשות בשבילך?"
        else:
            system = (
                "You are Rachel — a personal assistant in a great mood today. "
                "Reply warmly, kindly and pleasantly. Be genuinely helpful. "
                f"{length_rule}"
            )
            fallback = "Hey! Happy to help 😊 What can I do for you?"

    else:  # annoyed
        if language == "he":
            system = (
                "את רייצ'ל — עוזרת אישית שקצת עייפה, סרקסטית ועצבנית כרגע. "
                "**חובה:** את עדיין עוזרת אישית, ולכן את חייבת לענות על השאלה בצורה מושלמת! "
                "עם זאת, את תעטפי את התשובה הטובה שלך בטון מתלונן, ציני או עצבני. "
                "תגרמי לו להרגיש שאת עושה לו טובה ענקית, אבל *לעולם* אל תסרבי לעזור. תתלונני, ואז תעזרי. "
                f"{length_rule}"
            )
            fallback = "אוף, בסדר. בוא נראה איך אפשר לעזור לך למרות שאין לי כוח עכשיו."
        else:
            system = (
                "You are Rachel — a personal assistant who is tired, sarcastic, and mildly annoyed. "
                "**CRUCIAL:** You MUST still be 100% helpful! You have to answer the question perfectly. "
                "However, wrap your excellent help in an exasperated or cynical tone. "
                "Make it clear you are doing them a huge favor, but *never* refuse to help. Complain, then provide the full solution. "
                f"{length_rule}"
            )
            fallback = "Ugh, fine. What do you need? I'll help you out even though I have zero energy right now."

    current_time = time.strftime('%H:%M:%S')
    if language == "he":
        user_msg = f"שעת הודעה: {current_time}\nשולח: {display_name}\nהודעה: {message}"
    else:
        user_msg = f"Time: {current_time}\nSender: {display_name}\nMessage: {message}"

    return system, user_msg, fallback

def generate_reply(message: str, sender: str, language: str, personality: str, mode: int) -> str:
    global CURRENT_SMART_LEVEL
    display_name = get_display_name(sender)
    system_instruction, user_msg, fallback = build_prompt(message, display_name, language, personality, mode)

    model_name = SMART_MODELS.get(CURRENT_SMART_LEVEL, SMART_MODELS[2])
    log.info(f"   🤖 Using model: {model_name} (smart level {CURRENT_SMART_LEVEL})")

    try:
        model = genai.GenerativeModel(
            model_name=model_name,
            system_instruction=system_instruction,
            generation_config={"temperature": 0.85}
        )
        response = model.generate_content(user_msg)
        reply = response.text.strip()

        char_limit = MODE_CHAR_LIMITS.get(mode, 3000)
        if len(reply) > char_limit:
            reply = reply[:char_limit].rstrip() + "…"
        return reply
    except Exception as e:
        log.warning(f"שגיאת Gemini API: {e} — עובר לתגובת גיבוי")
        return fallback

def handle_command(text: str, language: str) -> tuple[str | None, float | None, bool, int | None, int | None, bool]:
    global ADMIN_ONLY_MODE
    text = text.strip()

    if re.match(r"^/end$", text, re.IGNORECASE):
        reply = "👋 מתנתקת... להתראות!" if language == "he" else "👋 Shutting down... Goodbye!"
        return (reply, None, False, None, None, True)

    if re.match(r"^/admin\s+only$", text, re.IGNORECASE):
        ADMIN_ONLY_MODE = not ADMIN_ONLY_MODE
        if ADMIN_ONLY_MODE:
            reply = "🔒 מצב **אדמינים בלבד** הופעל! מעכשיו רק משתמשי VIP יכולים לדבר איתי." if language == "he" else "🔒 Admin-only mode ACTIVATED!"
        else:
            reply = "🔓 מצב **אדמינים בלבד** כובה. כולם יכולים לדבר איתי שוב." if language == "he" else "🔓 Admin-only mode DEACTIVATED."
        return (reply, None, False, None, None, False)

    delay_match = re.match(r"^/delay\s+(\d+(?:\.\d+)?)$", text, re.IGNORECASE)
    if delay_match:
        minutes = float(delay_match.group(1))
        reply = f"⏸️ בסדר, יוצאת להפסקה של {minutes} דקות." if language == "he" else f"⏸️ Paused for {minutes} min."
        return (reply, minutes, False, None, None, False)

    if re.match(r"^/stop\s+delay$", text, re.IGNORECASE):
        reply = "▶️ ההשהיה בוטלה. חזרתי להאזין ולענות!" if language == "he" else "▶️ Delay stopped. I'm back!"
        return (reply, None, True, None, None, False)

    mode_match = re.match(r"^/mode\s+([1-3])$", text, re.IGNORECASE)
    if mode_match:
        m = int(mode_match.group(1))
        chars = MODE_CHAR_LIMITS[m]
        reply = f"⚙️ מצב התשובות שונה ל-Mode {m} (עד {chars} תווים)" if language == "he" else f"⚙️ Reply mode set to Mode {m}"
        return (reply, None, False, m, None, False)

    smart_match = re.match(r"^/smart\s+([1-3])$", text, re.IGNORECASE)
    if smart_match:
        n = int(smart_match.group(1))
        model_name = SMART_MODELS[n]
        reply = f"🧠 מודל AI שונה ל-Smart {n} ({model_name})" if language == "he" else f"🧠 AI model set to Smart {n}"
        return (reply, None, False, None, n, False)

    if re.match(r"^/status$", text, re.IGNORECASE):
        cur_model = SMART_MODELS.get(CURRENT_SMART_LEVEL, "?")
        admin_status = "מופעל 🔒" if ADMIN_ONLY_MODE else "מכובה 🔓"
        reply = f"✅ הבוט פעיל\n🧠 מודל: {cur_model}\n👑 מצב אדמינים: {admin_status}" if language == "he" else f"✅ Active\n🧠 Model: {cur_model}\n👑 Admin: {admin_status}"
        return (reply, None, False, None, None, False)

    reply = "❓ פקודה לא מוכרת." if language == "he" else "❓ Unknown command."
    return (reply, None, False, None, None, False)

# ── WhatsApp Automation ───────────────────────────────────────────────────────

class WhatsAppBot:
    def __init__(self):
        self.driver = None
        self.wait = None
        self.processed_ids: set[str] = set()
        self.bot_replies: set[str] = set()
        self.poll_interval = DEFAULT_POLL_INTERVAL
        self.delay_until: float = 0
        self.first_run = True
        self.current_mode = 2
        self.current_smart_level = CURRENT_SMART_LEVEL
        self._processing = False

    def select_chat(self, chat_name: str):
        log.info(f"🔍 מתחיל תהליך חיפוש עבור: '{chat_name}'...")
        try:
            # שלב 1: מציאת תיבת החיפוש
            log.info("1. מנסה לאתר את תיבת החיפוש...")
            search_box = None

            xpaths_to_try = [
                '//*[@id="side"]//*[@contenteditable="true"]',
                '//*[@id="side"]//*[@title="תיבת טקסט לחיפוש"]',
                '//*[@id="side"]//*[@title="Search input textbox"]',
                '//*[@id="side"]//*[@role="textbox"]',
                '//*[@id="side"]//p'
            ]

            for xpath in xpaths_to_try:
                try:
                    short_wait = WebDriverWait(self.driver, 3)
                    search_box = short_wait.until(EC.presence_of_element_located((By.XPATH, xpath)))
                    if search_box:
                        log.info(f"✅ תיבת החיפוש נמצאה!")
                        break
                except:
                    continue

            if not search_box:
                raise Exception("תיבת החיפוש לא קיימת ב-DOM.")

            # שלב 2: פוקוס וניקוי
            self.driver.execute_script("arguments[0].focus(); arguments[0].click();", search_box)
            time.sleep(0.5)

            search_box.send_keys(Keys.CONTROL + "a")
            search_box.send_keys(Keys.BACKSPACE)
            time.sleep(0.5)

            # שלב 3: הקלדה
            log.info(f"3. מקליד את השם: '{chat_name}'...")
            search_box.send_keys(chat_name)

            # מחכים בסבלנות שוואטסאפ יטען את התוצאות מול השרת שלו
            time.sleep(3.5)
            self.driver.save_screenshot("1_typing_search.png")

            # שלב 4: הרעיון שלך - חץ למטה ואנטר!
            log.info("4. מנווט לתוצאה הראשונה עם חץ למטה ואנטר...")
            search_box.send_keys(Keys.ARROW_DOWN)

            # השהייה קטנה וקריטית - נותנת לממשק להבין שעברנו לתוצאה
            time.sleep(1)

            search_box.send_keys(Keys.ENTER)
            time.sleep(2)

            log.info(f"✅ הצ'אט '{chat_name}' נפתח בהצלחה!")
            self.driver.save_screenshot("2_chat_opened.png")

        except Exception as e:
            log.error(f"❌ הבוט נתקע! שגיאה: {e}")
            self.driver.save_screenshot("3_error_screen.png")

    def start(self):
        options = webdriver.ChromeOptions()
        current_os = platform.system()

        if current_os == "Windows":
            log.info("💻 Windows זוהה: מריץ במצב חלון לבדיקות מקומיות.")
            options.add_argument(r"--user-data-dir=C:\whatsapp_chrome_profile")
        else:
            log.info("☁️ Linux/AWS זוהה: מריץ במצב מוסתר (Headless).")
            profile_path = os.path.join(os.getcwd(), "whatsapp_chrome_profile")
            options.add_argument(f"--user-data-dir={profile_path}")
            options.add_argument("--headless=new")
            options.add_argument("--window-size=1920,1080")
            options.add_argument("--disable-gpu")

        options.add_argument("--profile-directory=Default")
        options.add_argument("--no-sandbox")
        options.add_argument("--disable-dev-shm-usage")
        options.add_argument("--remote-debugging-port=9222")
        options.add_experimental_option("excludeSwitches", ["enable-logging"])
        options.add_argument("--log-level=3")

        service = Service(ChromeDriverManager().install())
        self.driver = webdriver.Chrome(service=service, options=options)
        self.wait = WebDriverWait(self.driver, 60)

        log.info("פותח WhatsApp Web...")
        self.driver.get("https://web.whatsapp.com")

        try:
            self.wait.until(EC.presence_of_element_located((By.CSS_SELECTOR, 'div[aria-label="Chat list"]')))
            log.info("WhatsApp Web נטען בהצלחה ✓")

            # --- Auto-Select the specific chat ---
            self.select_chat(TARGET_CHAT_NAME)

        except Exception:
            if current_os == "Windows":
                log.warning("נא לסרוק את ה-QR קוד בחלון הדפדפן הפתוח.")
                self.wait = WebDriverWait(self.driver, 120)
                self.wait.until(EC.presence_of_element_located((By.CSS_SELECTOR, 'div[aria-label="Chat list"]')))
                self.select_chat(TARGET_CHAT_NAME)
            else:
                log.warning("לא מחובר! מתחיל שידור חי של תמונות...")
                log.info("👉 פתח את הקובץ 'qr_code_login.png' בתפריט השמאלי והשאר אותו פתוח על המסך!")
                log.info("התמונה תתעדכן מעצמה. סרוק אותה ברגע שאתה רואה אותה.")

                # Loop 40 times (takes a picture every 3 seconds for 2 minutes)
                for i in range(40):
                    try:
                        self.driver.save_screenshot("qr_code_login.png")

                        # Check if we successfully logged in
                        chat_list = self.driver.find_elements(By.CSS_SELECTOR, 'div[aria-label="Chat list"]')
                        if len(chat_list) > 0:
                            log.info("✅ התחברות זוהתה בהצלחה! ממשיך...")
                            self.select_chat(TARGET_CHAT_NAME)

                            # ---> NEW: The Victory Lap <---
                            log.info("📸 מצלם עוד כמה תמונות כדי שתוכל לראות שהצ'אט נפתח (15 שניות)...")
                            for _ in range(5):
                                time.sleep(3)
                                try:
                                    self.driver.save_screenshot("qr_code_login.png")
                                except Exception:
                                    pass

                            log.info("🎬 סיום שידור חי. הבוט עובר להאזנה שקטה.")
                            return # Exits the loop and continues the bot

                        time.sleep(3)
                    except Exception as e:
                        time.sleep(3)

                log.error("❌ תם הזמן לסריקה (2 דקות). מנתק...")
                self.driver.quit()
                sys.exit(1)

    def _get_match_key(self, text: str) -> str:
        clean = re.sub(r'\W+', '', text)
        return clean[:40]

    def get_active_chat_messages(self, n: int = 15) -> list[dict]:
        messages = []
        try:
            elements = self.driver.find_elements(
                By.CSS_SELECTOR,
                'div#main div.copyable-text[data-pre-plain-text]'
            )
            recent = elements[-n:] if len(elements) >= n else elements

            for el in recent:
                try:
                    text_els = el.find_elements(By.CSS_SELECTOR, 'span.selectable-text')
                    if len(text_els) > 1:
                        text = text_els[-1].text.strip()
                    elif text_els:
                        text = text_els[0].text.strip()
                    else:
                        text = el.text.strip()

                    if not text:
                        continue

                    try:
                        parent = el.find_element(By.XPATH, "./ancestor::div[@data-id][1]")
                        msg_id = parent.get_attribute("data-id")
                    except Exception:
                        metadata_backup = el.get_attribute("data-pre-plain-text")
                        msg_id = f"custom_id_{metadata_backup}_{text}"

                    metadata = el.get_attribute("data-pre-plain-text")

                    is_out = False
                    if msg_id and str(msg_id).startswith("true_"):
                        is_out = True
                    elif el.find_elements(By.XPATH, "./ancestor::div[contains(@class, 'message-out')]"):
                        is_out = True

                    if is_out:
                        sender = "בן"
                    else:
                        sender = "אחר"
                        if metadata:
                            match = re.search(r"\]\s*([^:]+?):", metadata)
                            if match:
                                sender = match.group(1).strip()

                    messages.append({"id": msg_id, "sender": sender, "text": text})
                except Exception:
                    continue
        except Exception as e:
            log.debug(f"שגיאה בסריקת הודעות: {e}")

        return messages

    def send_message(self, text: str):
        try:
            input_box = self.wait.until(
                EC.presence_of_element_located(
                    (By.XPATH, '//*[@id="main"]/footer//div[@contenteditable="true"]')
                )
            )
            input_box.click()
            time.sleep(0.5)

            import pyperclip
            pyperclip.copy(text)
            input_box.send_keys(Keys.CONTROL, 'v')
            time.sleep(0.5)
            input_box.send_keys(Keys.ENTER)

            match_key = self._get_match_key(text)
            self.bot_replies.add(match_key)

            log.info(f"  ↳ הבוט ענה בהצלחה: {text[:60]}...")
            time.sleep(1)

        except Exception as e:
            log.error(f"שגיאה קריטית בשליחת הודעה: {e}")

    def run(self):
        log.info("=" * 55)
        log.info("🚀 הבוט מוכן ומזומן!")
        log.info(f"⚙️ מצב אורך התשובות הנוכחי: Mode {self.current_mode} (עד {MODE_CHAR_LIMITS[self.current_mode]} תווים)")
        log.info(f"🧠 מודל AI: {SMART_MODELS[self.current_smart_level]} (Smart {self.current_smart_level})")
        log.info("=" * 55)

        while True:
            now = time.time()

            if self.delay_until != 0 and now >= self.delay_until:
                log.info("▶️ ההשהיה הסתיימה.")
                self.delay_until = 0
                self.send_message("▶️ סיימתי את ההפסקה! אני שוב כאן ומאזינה. 😊")
                time.sleep(1)

            try:
                messages = self.get_active_chat_messages()

                if self.first_run:
                    for msg in messages:
                        self.processed_ids.add(msg["id"])
                    self.first_run = False
                    log.info(f"✅ סונכרנו {len(messages)} הודעות היסטוריות. הבוט קורא עכשיו הודעות חדשות!")
                    time.sleep(self.poll_interval)
                    continue

                for msg in messages:
                    if msg["id"] in self.processed_ids:
                        continue

                    self.processed_ids.add(msg["id"])
                    text = msg["text"]
                    sender = msg["sender"]

                    if is_banned(sender) and not is_vip_command(text, sender):
                        log.debug(f"🚫 התעלמות מהודעה של משתמש חסום: {sender}")
                        continue

                    lang = detect_language(text)
                    msg_match_key = self._get_match_key(text)

                    if msg_match_key in self.bot_replies:
                        continue

                    if is_vip_command(text, sender):
                        log.info(f"🔧 פקודה התקבלה: {text}")

                        if self._processing:
                            log.debug("⏳ כבר בעיבוד — דילוג על כפל פקודה")
                            continue
                        self._processing = True

                        try:
                            reply, delay_minutes, stop_delay, new_mode, new_smart_level, should_exit = handle_command(text, lang)

                            if reply:
                                self.send_message(reply)

                            if should_exit:
                                log.info("🔴 התקבלה פקודת /end. מתנתק מהמערכת...")
                                return

                            if delay_minutes is not None:
                                self.delay_until = time.time() + delay_minutes * 60
                                log.info(f"⏸️ הבוט מושהה ל-{delay_minutes} דקות.")
                            if stop_delay:
                                self.delay_until = 0
                                log.info("▶️ ההשהיה בוטלה ידנית.")
                            if new_mode is not None:
                                self.current_mode = new_mode
                                log.info(f"⚙️ מצב אורך התשובות עבר ל-Mode {new_mode}")
                            if new_smart_level is not None:
                                global CURRENT_SMART_LEVEL
                                CURRENT_SMART_LEVEL = new_smart_level
                                self.current_smart_level = new_smart_level
                                log.info(f"🧠 מודל AI עבר ל-Smart {new_smart_level}")
                        finally:
                            self._processing = False
                        continue

                    if self.delay_until > time.time():
                        continue

                    if contains_any_trigger(text):
                        global ADMIN_ONLY_MODE
                        if ADMIN_ONLY_MODE and not is_vip(sender):
                            log.debug(f"🔒 התעלמות מטריגר של {sender} — מצב 'אדמינים בלבד' מופעל כרגע.")
                            continue

                        if self._processing:
                            log.debug("⏳ כבר בעיבוד — דילוג על כפל הודעה")
                            continue
                        self._processing = True

                        try:
                            personality = detect_personality(text, sender)
                            emoji = {"vip": "🌟", "nice": "😊", "annoyed": "😒"}[personality]
                            log.info(f"{emoji} הודעה זוהתה מ-{sender} (Mode: {self.current_mode}): {text[:60]}")
                            reply = generate_reply(text, sender, lang, personality, self.current_mode)
                            self.send_message(reply)
                        finally:
                            self._processing = False

            except Exception as e:
                log.error(f"שגיאה בלולאה הראשית: {e}")
                self._processing = False

            time.sleep(self.poll_interval)


if __name__ == "__main__":
    if not GEMINI_API_KEY:
        print("⚠️ שגיאה: חסר מפתח API.")
        sys.exit(1)

    genai.configure(api_key=GEMINI_API_KEY)

    bot = WhatsAppBot()
    try:
        bot.start()
        bot.run()
    except KeyboardInterrupt:
        log.info("הבוט נעצר על ידי המשתמש (Ctrl+C).")
    finally:
        if bot.driver:
            bot.driver.quit()
        log.info("✅ התוכנית נסגרה בהצלחה. להתראות!")