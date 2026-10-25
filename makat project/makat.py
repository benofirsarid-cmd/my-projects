from rapidfuzz import process,fuzz_cpp as fuzz
from openpyxl import load_workbook,Workbook
from tqdm import tqdm
import os
from google import genai
from google.genai import types
from openpyxl.styles import PatternFill
import time
import concurrent.futures
stock = []
stock_makat = {}
makat_stock_to_new_makat = {}
product_to_new_name = {}
name_to_cell_row = {}
product_to_score = {}
client = genai.Client(api_key="AIzaSyD1UV7zJmuzAoLxEmVXbxVk8P6kAJ6ZJ9Y")
def insert_stuff():
    current = input("insert product (or 0 if done): ")
    current_makat = input("insert the makat(if 0 insert 0): ")
    while current!="0":
        stock.append(current)
        stock_makat[current]=current_makat
        current = input("insert product (or 0 if done): ")
        current_makat = input("insert the makat(if 0 insert 0): ")
    load_menu()

def compare_products_one():
    current = input("insert product (or 0 if done): ")
    current_makat = input("insert the makat(if 0 insert 0): ")
    largest_score = 0
    largest_score_name = ""
    while current!="0":
        try:
            for product in stock:
                score = fuzz.token_set_ratio(current, product)
                if score>largest_score:
                    largest_score = score
                    largest_score_name = product
            makat_stock_to_new_makat[stock_makat[largest_score_name]] = current_makat
            product_to_new_name[largest_score_name] = current
        except Exception as e:
            print("not found, please enter manually")
        current = input("insert product (or 0 if done): ")
        current_makat = input("insert the makat(if 0 insert 0): ")
        largest_score = 0
        largest_score_name = ""
    load_menu()

def compare_products_two():
    current = input("insert product (or 0 if done): ")
    current_makat = input("insert the makat(if 0 insert 0): ")
    largest_score = 0
    largest_score_name = ""
    while current!="0":
        try:
            for product in stock:
                score = fuzz.partial_ratio(current, product)
                if score>largest_score:
                    largest_score = score
                    largest_score_name = product
            makat_stock_to_new_makat[stock_makat[largest_score_name]] = current_makat
            product_to_new_name[largest_score_name] = current
        except Exception as e:
            print("not found, please enter manually")
        current = input("insert product (or 0 if done): ")
        current_makat = input("insert the makat(if 0 insert 0): ")
        largest_score = 0
        largest_score_name = ""
    load_menu()

def compare_products_three():
    wb = load_workbook('data2.xlsx')
    sheet = wb.active
    for cell in sheet['A']:
        if cell.row >= 2 and cell.value is not None:
            try:
                for product in stock:
                  score = fuzz.partial_ratio(cell.value, product)
                  if score>largest_score:
                       largest_score = score
                       largest_score_name = product
                makat_stock_to_new_makat[stock_makat[largest_score_name]] = cell.offset(row=0, column=1).value
                product_to_new_name[largest_score_name] = cell.value
            except Exception as e:
                print("not found, please enter manually")
        largest_score = 0
        largest_score_name = ""
    for cell in sheet['A']:
        if cell.row >= 2 and cell.value is not None:
            try:
                for product in stock:
                  score = fuzz.token_set_ratio(cell.value, product)
                  if score>largest_score:
                       largest_score = score
                       largest_score_name = product
                makat_stock_to_new_makat[stock_makat[largest_score_name]] = cell.offset(row=0, column=1).value
                product_to_new_name[largest_score_name] = cell.value
            except Exception as e:
                print("not found, please enter manually")
        largest_score = 0
        largest_score_name = ""
    load_menu()


def ai_choose_best_match(our_product, options):
    options_text = "\n".join([f"{i + 1}. {opt}" for i, opt in enumerate(options)])

    prompt = f"""You need to match a product from our store to the EXACT SAME product from a supplier's catalog.

Our product: "{our_product}"

Supplier options:
{options_text}

If one of the options is the EXACT SAME product, reply ONLY with its number (between 1 and {len(options)}).
If NONE of them are the same product, reply ONLY with '0'."""

    try:
        # הוספת השהיה קטנה לפני כל פנייה כדי לא לחסום את ה-API בגרסת החינם
        time.sleep(4.1)

        response = client.models.generate_content(
            model='gemini-2.0-flash',  # דגם מהיר ומעולה לגרסת החינם
            contents=prompt,
            config=types.GenerateContentConfig(
                temperature=0.0,
                system_instruction="You are a strict data-matching assistant. Reply ONLY with a single number."
            )
        )
        answer = response.text.strip()
        if answer.isdigit():
            choice = int(answer)
            if 1 <= choice <= len(options):
                return options[choice - 1]
        return None
    except Exception as e:
        print(f"Error: {e}")
        return None


def compare_products_three_two():
    print("Loading data... (Free Tier Mode - with rate limiting)")
    wb = load_workbook('data2.xlsx')
    sheet = wb.active

    external_products_dict = {str(cell.value): cell.offset(row=0, column=1).value
                              for cell in sheet['A'] if cell.row >= 2 and cell.value}
    external_names = list(external_products_dict.keys())

    matched_ours = set()
    matched_exts = set()

    # ריצה על המוצרים (אחד אחד עם השהיה)
    for our_product in tqdm(stock, desc="Matching (Free Tier)", unit="product"):
        if our_product in matched_ours: continue

        # שולפים 90 מועמדים
        best_matches = process.extract(our_product, external_names, scorer=fuzz.token_set_ratio, limit=90)
        available_matches = [m for m in best_matches if m[0] not in matched_exts]

        if not available_matches or available_matches[0][1] < 30:
            product_to_score[our_product] = 0
            continue

        is_match = False
        final_ext_name = None

        if available_matches[0][1] == 100:
            is_match = True
            final_ext_name = available_matches[0][0]
        else:
            candidates = [m[0] for m in available_matches]
            # חלוקה ל-3 קבוצות של 30
            for i in range(0, len(candidates), 30):
                chunk = candidates[i: i + 30]
                ai_choice = ai_choose_best_match(our_product, chunk)
                if ai_choice:
                    is_match = True
                    final_ext_name = ai_choice
                    break

        if is_match:
            matched_ours.add(our_product)
            matched_exts.add(final_ext_name)
            product_to_score[our_product] = 100
            # עדכון המק"טים (לפי הלוגיקה שלך)
            our_makat = stock_makat.get(our_product)
            if our_makat:
                product_to_new_name[our_product] = final_ext_name
                makat_stock_to_new_makat[our_makat] = external_products_dict[final_ext_name]
        else:
            product_to_score[our_product] = 0

    print(f"\nDone! Matched {len(matched_ours)} products.")
    load_menu()


def get_top_candidates(our_product, external_names):
    """פונקציה שרצה במקביל על כל ליבות המעבד עבור ה-Fuzz בלבד"""
    matches = process.extract(
        our_product,
        external_names,
        scorer=fuzz.token_set_ratio,
        limit=90
    )
    return our_product, matches


def ai_choose_best_match(our_product, options):
    """פונקציית ה-AI - רצה באופן טורי (לא מקבילי)"""
    options_text = "\n".join([f"{i + 1}. {opt}" for i, opt in enumerate(options)])
    prompt = f"""Match our product to the supplier's catalog.
Our product: "{our_product}"
Supplier options:
{options_text}
Reply ONLY with the number of the exact match, or '0' if none match."""

    try:
        # אם את ב-Free Tier, אפשר להוסיף כאן time.sleep(4) כדי למנוע חסימה
        response = client.models.generate_content(
            model='gemini-2.0-flash',
            contents=prompt,
            config=types.GenerateContentConfig(
                temperature=0.0,
                system_instruction="Reply ONLY with a single number."
            )
        )
        answer = response.text.strip()
        if answer.isdigit():
            choice = int(answer)
            if 1 <= choice <= len(options):
                return options[choice - 1]
        return None
    except Exception:
        return None


def compare_products_three_two0():
    print("Loading data...")
    wb = load_workbook('data2.xlsx')
    sheet = wb.active

    external_products_dict = {str(cell.value): cell.offset(row=0, column=1).value
                              for cell in sheet['A'] if cell.row >= 2 and cell.value}
    external_names = list(external_products_dict.keys())

    # --- שלב 1: FUZZ במקביליות (Multi-Processing) ---
    print(f"Phase 1: Running Fuzz on all CPU cores...")
    all_candidates = {}

    # ProcessPoolExecutor משתמש בכל הליבות של המעבד לחישוב מהיר
    with concurrent.futures.ProcessPoolExecutor() as executor:
        futures = [executor.submit(get_top_candidates, p, external_names) for p in stock]
        for future in tqdm(concurrent.futures.as_completed(futures), total=len(futures), desc="Fuzzing"):
            prod, matches = future.result()
            all_candidates[prod] = matches

    # --- שלב 2: AI בטור (Single Threaded) ---
    print(f"\nPhase 2: AI Validation (One by one)...")
    matched_ours = set()
    matched_exts = set()

    # עוברים על כל המוצרים אחד אחד בצורה רגילה
    for our_product in tqdm(stock, desc="AI Processing"):
        candidates_data = all_candidates.get(our_product, [])

        # סינון מועמדים שכבר נתפסו על ידי מוצרים קודמים בלולאה
        available = [m for m in candidates_data if m[0] not in matched_exts]

        if not available or available[0][1] < 30:
            product_to_score[our_product] = 0
            continue

        is_match = False
        final_ext_name = None

        # בדיקה אם יש 100% פאז
        if available[0][1] == 100:
            is_match = True
            final_ext_name = available[0][0]
        else:
            # חלוקה לקבוצות של 30 עבור ה-AI
            names_only = [m[0] for m in available]
            for i in range(0, len(names_only), 30):
                chunk = names_only[i: i + 30]
                ai_choice = ai_choose_best_match(our_product, chunk)
                if ai_choice:
                    is_match = True
                    final_ext_name = ai_choice
                    break

        # שמירת תוצאות
        if is_match and final_ext_name:
            matched_ours.add(our_product)
            matched_exts.add(final_ext_name)

            our_makat = stock_makat.get(our_product)
            if our_makat:
                product_to_new_name[our_product] = final_ext_name
                makat_stock_to_new_makat[our_makat] = external_products_dict[final_ext_name]
                product_to_score[our_product] = 100
        else:
            product_to_score[our_product] = 0

    print(f"\n✅ Finished! Matched {len(matched_ours)} products.")
    input("Press Enter to return to menu...")
    load_menu()

def show_makats_two():
    compared_count = 0
    not_compared_count = 0
    for product in stock:
        if product in product_to_new_name and product_to_new_name[product] is not None:
            compared_count += 1
            our_makat = stock_makat.get(product, "N/A")
            other_name = product_to_new_name[product]
            other_makat = makat_stock_to_new_makat.get(our_makat, "N/A")
            print(f"Product: {product} | Our Code: {our_makat}")
            print(f"Other Name: {other_name} | Other Code: {other_makat}")
            print("-" * 20)
        else:
            not_compared_count += 1
            print(f"Product: {product} | Status: NOT COMPARED")
            print("-" * 20)
    print("\n" + "=" * 30)
    print(f"Comparison Summary:")
    print(f"Total products in stock: {len(stock)}")
    print(f"Successfully compared: {compared_count}")
    print(f"Not compared: {not_compared_count}")
    print("=" * 30 + "\n")
    input("Type 1 to continue: ")
    load_menu()

def show_makats():
    for product in stock:
        print("product name: " + product + " our code: " + stock_makat[product])
        print("other name: " + product_to_new_name[product] + " other code: " + makat_stock_to_new_makat[stock_makat[product]])
    input("type 1 to continue: ")
    load_menu()

def manual_entry():
    name = input("enter our name: ")
    makat = input("enter our makat: ")
    new_name = input("enter their name: ")
    new_makat = input("enter their makat: ")
    if name not in stock:
        stock.append(name)
        stock_makat[name] = makat
    makat_stock_to_new_makat[makat] = new_makat
    product_to_new_name[name] = new_name
    load_menu()

def export_to_file():
    with open("products.txt", "w", encoding="utf-8") as f:
        for product in stock:
            if product_to_new_name[product] != None:
                f.write("product name: " + str(product) + " new name: " + str(product_to_new_name[product]) + " makat: " + str(stock_makat[product]) + " new makat: " + str(makat_stock_to_new_makat[stock_makat[product]]) + "\n")
    load_menu()


def export_to_excel():
    wb = Workbook()
    ws = wb.active
    ws.title = "Compared Products"
    ws.append(["Our Product Name", "Our Makat", "Matched Product", "Matched Makat", "Match Score"])
    green_fill = PatternFill(start_color="C6EFCE", end_color="C6EFCE", fill_type="solid")
    yellow_fill = PatternFill(start_color="FFEB9C", end_color="FFEB9C", fill_type="solid")
    red_fill = PatternFill(start_color="FFC7CE", end_color="FFC7CE", fill_type="solid")
    data_to_export = []
    green_count = 0
    yellow_count = 0
    red_count = 0
    for product in stock:
        if product in product_to_new_name:
            our_makat = stock_makat.get(product, "N/A")
            matched_name = product_to_new_name[product]
            matched_makat = makat_stock_to_new_makat.get(our_makat, "N/A")
            score = product_to_score.get(product, 0)
            if score > 80:
                green_count += 1
            elif score >= 50:
                yellow_count += 1
            else:
                red_count += 1
            data_to_export.append((product, our_makat, matched_name, matched_makat, score))
    data_to_export.sort(key=lambda x: x[4], reverse=True)
    for item in data_to_export:
        product, our_makat, matched_name, matched_makat, score = item
        bars_count = int(score) // 10
        bar_string = "█" * bars_count + "░" * (10 - bars_count)
        display_score = f"{bar_string} {int(score)}%"
        ws.append([product, our_makat, matched_name, matched_makat, display_score])
        current_row = ws.max_row
        score_cell = ws.cell(row=current_row, column=5)
        if score > 80:
            score_cell.fill = green_fill
        elif score >= 50:
            score_cell.fill = yellow_fill
        else:
            score_cell.fill = red_fill
    for col in ws.columns:
        max_length = 0
        column = col[0].column_letter
        for cell in col:
            try:
                if len(str(cell.value)) > max_length:
                    max_length = len(str(cell.value))
            except:
                pass
        ws.column_dimensions[column].width = max_length + 2
    wb.save("compared_products.xlsx")
    print("\n" + "=" * 45)
    print(" Exported successfully to 'compared_products.xlsx'!")
    print("=" * 45)
    print(f" 🟩 Green matches (>80%):    {green_count}")
    print(f" 🟨 Yellow matches (50-80%): {yellow_count}")
    print(f" 🟥 Red matches (<50%):      {red_count}")
    print("=" * 45 + "\n")

    input("Press Enter to return to menu...")
    load_menu()

def load_menu():
    os.system('cls')
    print("choose what to do:")
    print("1. insert products")
    print("2. compare products")
    print("3. print compared codes")
    print("4. enter manually")
    print("5. export to excel file")
    print("6. to end program")
    choise = int(input("choose an option: "))
    os.system('cls')
    match choise:
        case 1:
            insert_stuff()
        case 2:
            met = int(input("choose method (1 ,2 or 3)"))
            if met == 1:
                compare_products_one()
            elif met == 2:
                compare_products_two()
            elif met ==3:
                compare_products_three_two0()
        case 3:
            show_makats_two()
        case 4:
            manual_entry()
        case 5:
            export_to_excel()
        case 6:
            main(0)

def main(x):
    if x!=0:
        wb = load_workbook('data.xlsx')
        sheet = wb.active
        for cell in sheet['C']:
            if cell.row >= 2 and cell.value is not None:
                stock.append(cell.value)
                name_to_cell_row[cell.value] = cell.row
                stock_makat[cell.value] = cell.offset(row=0, column=-1).value
        load_menu()


if __name__ == '__main__':
    main(1)