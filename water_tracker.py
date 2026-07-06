# water_tracker.py
import json
from dataclasses import dataclass, asdict
from datetime import datetime, timedelta
from typing import List, Optional
from pathlib import Path

@dataclass
class WaterEntry:
    id: int
    timestamp: str
    amount: int  # мл
    note: str = ""

class WaterTracker:
    def __init__(self):
        self.entries: List[WaterEntry] = []
        self.next_id = 1
        self.daily_goal = 2000  # мл

    def add_entry(self, amount: int, note: str = "") -> WaterEntry:
        if amount <= 0:
            raise ValueError("Объём должен быть положительным числом.")
        entry = WaterEntry(
            id=self.next_id,
            timestamp=datetime.now().isoformat(),
            amount=amount,
            note=note
        )
        self.entries.append(entry)
        self.next_id += 1
        return entry

    def find_entry(self, entry_id: int) -> Optional[WaterEntry]:
        return next((e for e in self.entries if e.id == entry_id), None)

    def delete_entry(self, entry_id: int) -> bool:
        entry = self.find_entry(entry_id)
        if entry:
            self.entries.remove(entry)
            return True
        return False

    def get_today_entries(self) -> List[WaterEntry]:
        today = datetime.now().date()
        return [e for e in self.entries if datetime.fromisoformat(e.timestamp).date() == today]

    def get_week_entries(self) -> List[WaterEntry]:
        week_ago = datetime.now() - timedelta(days=7)
        return [e for e in self.entries if datetime.fromisoformat(e.timestamp) >= week_ago]

    def get_stats(self) -> dict:
        today_entries = self.get_today_entries()
        week_entries = self.get_week_entries()
        today_total = sum(e.amount for e in today_entries)
        week_total = sum(e.amount for e in week_entries)
        week_days = 7
        avg_daily = week_total / week_days if week_entries else 0
        progress = (today_total / self.daily_goal * 100) if self.daily_goal > 0 else 0
        return {
            "today_total": today_total,
            "today_count": len(today_entries),
            "week_total": week_total,
            "week_count": len(week_entries),
            "avg_daily": avg_daily,
            "daily_goal": self.daily_goal,
            "progress": min(progress, 100)
        }

    def save_to_file(self, filename: str = "water_data.json") -> None:
        data = {
            "daily_goal": self.daily_goal,
            "entries": [asdict(e) for e in self.entries]
        }
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

    def load_from_file(self, filename: str = "water_data.json") -> None:
        path = Path(filename)
        if not path.exists():
            return
        with open(filename, "r", encoding="utf-8") as f:
            data = json.load(f)
            self.daily_goal = data.get("daily_goal", 2000)
            self.entries.clear()
            for item in data.get("entries", []):
                entry = WaterEntry(
                    id=item["id"],
                    timestamp=item["timestamp"],
                    amount=item["amount"],
                    note=item.get("note", "")
                )
                self.entries.append(entry)
                if entry.id >= self.next_id:
                    self.next_id = entry.id + 1

def print_entry(entry: WaterEntry) -> None:
    print(f"#{entry.id} - {entry.timestamp} - {entry.amount} мл")
    if entry.note:
        print(f"   Заметка: {entry.note}")

def main():
    tracker = WaterTracker()
    tracker.load_from_file()

    while True:
        print("\n===== ТРЕКЕР ВОДНОГО БАЛАНСА (Python) =====")
        print("1. Добавить запись")
        print("2. Просмотреть записи за сегодня")
        print("3. Просмотреть записи за неделю")
        print("4. Показать статистику")
        print("5. Установить дневную цель")
        print("6. Удалить запись")
        print("7. Сохранить в файл")
        print("8. Загрузить из файла")
        print("0. Выход")
        choice = input("Выберите действие: ").strip()

        if choice == "0":
            break
        elif choice == "1":
            try:
                amount = int(input("Объём (мл): ").strip())
            except ValueError:
                print("Введите число.")
                continue
            if amount <= 0:
                print("Объём должен быть положительным.")
                continue
            note = input("Заметка (необязательно): ").strip()
            entry = tracker.add_entry(amount, note)
            print(f"Запись добавлена с ID {entry.id}")
        elif choice == "2":
            entries = tracker.get_today_entries()
            if not entries:
                print("За сегодня записей нет.")
            else:
                print(f"\nЗаписи за сегодня ({len(entries)}):")
                for e in entries:
                    print_entry(e)
        elif choice == "3":
            entries = tracker.get_week_entries()
            if not entries:
                print("За неделю записей нет.")
            else:
                print(f"\nЗаписи за последние 7 дней ({len(entries)}):")
                for e in entries:
                    print_entry(e)
        elif choice == "4":
            stats = tracker.get_stats()
            print("\n=== СТАТИСТИКА ===")
            print(f"Дневная цель: {stats['daily_goal']} мл")
            print(f"Сегодня выпито: {stats['today_total']} мл ({stats['today_count']} записей)")
            print(f"Прогресс: {stats['progress']:.1f}%")
            print(f"За неделю: {stats['week_total']} мл ({stats['week_count']} записей)")
            print(f"Среднее за день (за неделю): {stats['avg_daily']:.1f} мл")
        elif choice == "5":
            try:
                new_goal = int(input("Новая дневная цель (мл): ").strip())
            except ValueError:
                print("Введите число.")
                continue
            if new_goal <= 0:
                print("Цель должна быть положительной.")
                continue
            tracker.daily_goal = new_goal
            print(f"Цель установлена: {new_goal} мл")
        elif choice == "6":
            try:
                entry_id = int(input("Введите ID записи для удаления: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            if tracker.delete_entry(entry_id):
                print("Запись удалена.")
            else:
                print("Запись не найдена.")
        elif choice == "7":
            tracker.save_to_file()
            print("Сохранено.")
        elif choice == "8":
            tracker.load_from_file()
            print("Загружено.")
        else:
            print("Неизвестная команда.")

if __name__ == "__main__":
    main()
