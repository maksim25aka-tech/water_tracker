// water_tracker.go
package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
)

type WaterEntry struct {
	ID        int       `json:"id"`
	Timestamp time.Time `json:"timestamp"`
	Amount    int       `json:"amount"`
	Note      string    `json:"note"`
}

type WaterData struct {
	DailyGoal int          `json:"daily_goal"`
	Entries   []WaterEntry `json:"entries"`
}

type WaterTracker struct {
	entries   []WaterEntry
	nextID    int
	dailyGoal int
}

func NewWaterTracker() *WaterTracker {
	return &WaterTracker{
		entries:   []WaterEntry{},
		nextID:    1,
		dailyGoal: 2000,
	}
}

func (t *WaterTracker) AddEntry(amount int, note string) (WaterEntry, error) {
	if amount <= 0 {
		return WaterEntry{}, fmt.Errorf("объём должен быть положительным")
	}
	entry := WaterEntry{
		ID:        t.nextID,
		Timestamp: time.Now(),
		Amount:    amount,
		Note:      note,
	}
	t.entries = append(t.entries, entry)
	t.nextID++
	return entry, nil
}

func (t *WaterTracker) FindEntry(id int) *WaterEntry {
	for i := range t.entries {
		if t.entries[i].ID == id {
			return &t.entries[i]
		}
	}
	return nil
}

func (t *WaterTracker) DeleteEntry(id int) bool {
	for i, e := range t.entries {
		if e.ID == id {
			t.entries = append(t.entries[:i], t.entries[i+1:]...)
			return true
		}
	}
	return false
}

func (t *WaterTracker) TodayEntries() []WaterEntry {
	now := time.Now()
	today := time.Date(now.Year(), now.Month(), now.Day(), 0, 0, 0, 0, now.Location())
	var result []WaterEntry
	for _, e := range t.entries {
		if e.Timestamp.After(today) || e.Timestamp.Equal(today) {
			result = append(result, e)
		}
	}
	return result
}

func (t *WaterTracker) WeekEntries() []WaterEntry {
	weekAgo := time.Now().AddDate(0, 0, -7)
	var result []WaterEntry
	for _, e := range t.entries {
		if e.Timestamp.After(weekAgo) || e.Timestamp.Equal(weekAgo) {
			result = append(result, e)
		}
	}
	return result
}

func (t *WaterTracker) Stats() map[string]interface{} {
	today := t.TodayEntries()
	week := t.WeekEntries()
	todayTotal := 0
	for _, e := range today {
		todayTotal += e.Amount
	}
	weekTotal := 0
	for _, e := range week {
		weekTotal += e.Amount
	}
	avgDaily := float64(weekTotal) / 7.0
	progress := float64(todayTotal) / float64(t.dailyGoal) * 100
	if progress > 100 {
		progress = 100
	}
	return map[string]interface{}{
		"today_total": todayTotal,
		"today_count": len(today),
		"week_total":  weekTotal,
		"week_count":  len(week),
		"avg_daily":   avgDaily,
		"daily_goal":  t.dailyGoal,
		"progress":    progress,
	}
}

func (t *WaterTracker) SetDailyGoal(goal int) error {
	if goal <= 0 {
		return fmt.Errorf("цель должна быть положительной")
	}
	t.dailyGoal = goal
	return nil
}

func (t *WaterTracker) SaveToFile(filename string) error {
	data := WaterData{
		DailyGoal: t.dailyGoal,
		Entries:   t.entries,
	}
	jsonData, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filename, jsonData, 0644)
}

func (t *WaterTracker) LoadFromFile(filename string) error {
	data, err := os.ReadFile(filename)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}
	var waterData WaterData
	if err := json.Unmarshal(data, &waterData); err != nil {
		return err
	}
	t.dailyGoal = waterData.DailyGoal
	t.entries = waterData.Entries
	for _, e := range t.entries {
		if e.ID >= t.nextID {
			t.nextID = e.ID + 1
		}
	}
	return nil
}

func readString(prompt string) string {
	fmt.Print(prompt)
	reader := bufio.NewReader(os.Stdin)
	input, _ := reader.ReadString('\n')
	return strings.TrimSpace(input)
}

func readInt(prompt string) int {
	for {
		input := readString(prompt)
		if val, err := strconv.Atoi(input); err == nil {
			return val
		}
		fmt.Println("Введите число.")
	}
}

func printEntry(entry WaterEntry) {
	fmt.Printf("#%d - %s - %d мл\n", entry.ID, entry.Timestamp.Format("2006-01-02 15:04:05"), entry.Amount)
	if entry.Note != "" {
		fmt.Printf("   Заметка: %s\n", entry.Note)
	}
}

func main() {
	tracker := NewWaterTracker()
	if err := tracker.LoadFromFile("water_data.json"); err != nil {
		fmt.Println("Ошибка загрузки:", err)
	}

	for {
		fmt.Println("\n===== ТРЕКЕР ВОДНОГО БАЛАНСА (Go) =====")
		fmt.Println("1. Добавить запись")
		fmt.Println("2. Просмотреть записи за сегодня")
		fmt.Println("3. Просмотреть записи за неделю")
		fmt.Println("4. Показать статистику")
		fmt.Println("5. Установить дневную цель")
		fmt.Println("6. Удалить запись")
		fmt.Println("7. Сохранить в файл")
		fmt.Println("8. Загрузить из файла")
		fmt.Println("0. Выход")
		choice := readString("Выберите действие: ")

		switch choice {
		case "0":
			return
		case "1":
			amount := readInt("Объём (мл): ")
			if amount <= 0 {
				fmt.Println("Объём должен быть положительным.")
				continue
			}
			note := readString("Заметка (необязательно): ")
			entry, err := tracker.AddEntry(amount, note)
			if err != nil {
				fmt.Println(err)
				continue
			}
			fmt.Printf("Запись добавлена с ID %d\n", entry.ID)
		case "2":
			entries := tracker.TodayEntries()
			if len(entries) == 0 {
				fmt.Println("За сегодня записей нет.")
			} else {
				fmt.Printf("\nЗаписи за сегодня (%d):\n", len(entries))
				for _, e := range entries {
					printEntry(e)
				}
			}
		case "3":
			entries := tracker.WeekEntries()
			if len(entries) == 0 {
				fmt.Println("За неделю записей нет.")
			} else {
				fmt.Printf("\nЗаписи за последние 7 дней (%d):\n", len(entries))
				for _, e := range entries {
					printEntry(e)
				}
			}
		case "4":
			stats := tracker.Stats()
			fmt.Println("\n=== СТАТИСТИКА ===")
			fmt.Printf("Дневная цель: %d мл\n", stats["daily_goal"])
			fmt.Printf("Сегодня выпито: %d мл (%d записей)\n", stats["today_total"], stats["today_count"])
			fmt.Printf("Прогресс: %.1f%%\n", stats["progress"])
			fmt.Printf("За неделю: %d мл (%d записей)\n", stats["week_total"], stats["week_count"])
			fmt.Printf("Среднее за день (за неделю): %.1f мл\n", stats["avg_daily"])
		case "5":
			goal := readInt("Новая дневная цель (мл): ")
			if err := tracker.SetDailyGoal(goal); err != nil {
				fmt.Println(err)
				continue
			}
			fmt.Printf("Цель установлена: %d мл\n", goal)
		case "6":
			id := readInt("Введите ID записи для удаления: ")
			if tracker.DeleteEntry(id) {
				fmt.Println("Запись удалена.")
			} else {
				fmt.Println("Запись не найдена.")
			}
		case "7":
			if err := tracker.SaveToFile("water_data.json"); err != nil {
				fmt.Println("Ошибка сохранения:", err)
			} else {
				fmt.Println("Сохранено.")
			}
		case "8":
			if err := tracker.LoadFromFile("water_data.json"); err != nil {
				fmt.Println("Ошибка загрузки:", err)
			} else {
				fmt.Println("Загружено.")
			}
		default:
			fmt.Println("Неизвестная команда.")
		}
	}
}
