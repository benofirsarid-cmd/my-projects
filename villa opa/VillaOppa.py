import tkinter as tk
from tkinter import messagebox
import pandas as pd
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.keys import Keys
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.common.action_chains import ActionChains
import time
import os
import subprocess

EXCEL_FILE = 'WhatsApp_Contacts_ALL.xlsx'
IMAGE_FILES = ['villaopa1.jpg', 'villaopa2.jpg','villaopa3.jpg']
LANDING_PAGE_URL = "https://villa-oppa.netlify.app/"


class WhatsAppAutomator:
    def __init__(self, start_row, end_row):
        self.start_row = start_row
        self.end_row = end_row
        self.driver = None
        self.rows_to_drop = []

    def start(self):
        try:
            self.df = pd.read_excel(EXCEL_FILE)
        except FileNotFoundError:
            messagebox.showerror("שגיאה", f"לא נמצא הקובץ {EXCEL_FILE}")
            return

        # הפעלת הדפדפן
        self.driver = webdriver.Chrome()
        self.driver.get("https://web.whatsapp.com")

        messagebox.showinfo("התחברות",
                            "סרוק את הברקוד בוואטסאפ, המתן שכל השיחות ייטענו במלואן, ואז לחץ OK. (החלון יכול להישאר ברקע)")

        for index in range(self.start_row, self.end_row):
            if index >= len(self.df):
                break

            phone = str(self.df.iloc[index, 0])
            phone = "".join(filter(str.isdigit, phone))

            print(f"\n[{phone}] מעבד מספר...")
            success = self.process_contact(phone)

            if not success:
                print(f"[{phone}] נכשל. עובר למספר הבא.")

        if self.rows_to_drop:
            self.df = self.df.drop(self.rows_to_drop)
            self.df.to_excel(EXCEL_FILE, index=False)
            print(f"הוסרו {len(self.rows_to_drop)} שורות שהתבקשו להסרה.")

        self.driver.quit()
        messagebox.showinfo("סיום", "הקמפיין הסתיים בהצלחה!")

    def process_contact(self, phone):
        wait = WebDriverWait(self.driver, 20)
        url = f"https://web.whatsapp.com/send?phone={phone}"

        try:
            try:
                self.driver.switch_to.alert.accept()
            except:
                pass

            self.driver.get(url)

            # המתנה לתיבת הטקסט כדי לוודא שהצ'אט נטען לחלוטין
            WebDriverWait(self.driver, 60).until(
                EC.presence_of_element_located((By.XPATH, '//footer//div[@contenteditable="true"]'))
            )
            time.sleep(2)

            # בדיקת הסרה
            messages = self.driver.find_elements(By.CSS_SELECTOR, "div.message-in span.selectable-text")
            if any("הסר" in msg.text or "Remove" in msg.text for msg in messages):
                print(f"[{phone}] זוהתה בקשת הסרה.")
                self.rows_to_drop.append(self.df[self.df.iloc[:, 0] == phone].index[0])
                return True

            # העלאת תמונות - בשיטת ההעתק-הדבק ת
            for img in IMAGE_FILES:
                if not self.send_image(img, wait):
                    return False

            # שליחת טקסט
            if not self.send_text(wait):
                return False

            print(f"[{phone}] V הושלם בהצלחה")
            return True

        except Exception as e:
            print(f"[{phone}] שגיאה כללית במעבר למספר: {e}")
            return False

    def send_image(self, img_name, wait):
        img_path = os.path.abspath(img_name)
        if not os.path.exists(img_path):
            print(f"תמונה {img_name} חסרה.")
            return True

        try:
            # שלב 1: העתקת הקובץ ללוח (Clipboard) של ווינדוס בעזרת PowerShell
            print(f"  -> מעתיק את התמונה '{img_name}' ללוח...")
            subprocess.run(["powershell", "-command", f"Set-Clipboard -Path '{img_path}'"], capture_output=True)
            time.sleep(1)  # נותנים למערכת ההפעלה שניה לעדכן את הלוח

            # שלב 2: הדבקה ישירה בתיבת הטקסט
            print("  -> מדביק את התמונה בצ'אט (CTRL+V)...")
            msg_box = wait.until(EC.element_to_be_clickable((By.XPATH, '//footer//div[@contenteditable="true"]')))
            msg_box.click()  # פוקוס על התיבה
            msg_box.send_keys(Keys.CONTROL, 'v')  # הדבקה

            time.sleep(2.5)  # המתנה לטעינת התצוגה המקדימה של התמונה בוואטסאפ

            # שלב 3: איתור ולחיצה על כפתור השליחה במסך התצוגה
            print("  -> לוחץ enter לשליחת התמונה")
            actions = ActionChains(self.driver)
            actions.send_keys(Keys.ENTER).perform()
            time.sleep(3.5)
            return True

        except TimeoutException:
            print(f"  -> טיימאאוט: כפתור השליחה לא הופיע לאחר ההדבקה.")
            return False
        except Exception as e:
            print(f"  -> שגיאה בהדבקת או שליחת התמונה: {e}")
            return False

    def send_text(self, wait):
        try:
            msg_box = wait.until(EC.presence_of_element_located((By.XPATH, '//footer//div[@contenteditable="true"]')))
            msg_box.send_keys(f"לכל הפרטים, תמונות נוספות ומחירון היכנסו לקישור:\n{LANDING_PAGE_URL}")
            time.sleep(1.5)

            send_xpath = '//*[@data-icon="send"]/.. | //button[contains(@aria-label, "שליחה") or contains(@aria-label, "Send")]'
            send_btn = wait.until(EC.element_to_be_clickable((By.XPATH, send_xpath)))
            send_btn.click()
            time.sleep(4)

            return True

        except TimeoutException:
            print("  -> טיימאאוט באיתור או שליחת הטקסט.")
            return False
        except Exception as e:
            print(f"  -> שגיאה בשליחת טקסט: {e}")
            return False


def run_app():
    try:
        start_r = int(entry_start.get()) - 2
        end_r = int(entry_end.get()) - 1
        if start_r < 0:
            raise ValueError
    except ValueError:
        messagebox.showerror("שגיאה", "אנא הכנס מספרים תקינים לשורות.")
        return

    bot = WhatsAppAutomator(start_r, end_r)
    bot.start()


root = tk.Tk()
root.title("מערכת הפצה - וילה אופא")
root.geometry("350x250")

tk.Label(root, text=":התחל משורה מספר (באקסל)", font=("Arial", 12)).pack(pady=5)
entry_start = tk.Entry(root, font=("Arial", 12), justify="center")
entry_start.pack(pady=5)

tk.Label(root, text=":עד שורה מספר", font=("Arial", 12)).pack(pady=5)
entry_end = tk.Entry(root, font=("Arial", 12), justify="center")
entry_end.pack(pady=5)

tk.Button(root, text="התחל אוטומציה", font=("Arial", 14, "bold"), bg="#27ae60", fg="white", command=run_app).pack(
    pady=20)

root.mainloop()