// WaterTracker.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

public record WaterEntry(
    int Id,
    string Timestamp,
    int Amount,
    string Note
);

public class WaterData
{
    public int DailyGoal { get; set; }
    public List<WaterEntry> Entries { get; set; } = new();
}

public class WaterTracker
{
    private List<WaterEntry> entries = new();
    private int nextId = 1;
    private int dailyGoal = 2000;

    public IReadOnlyList<WaterEntry> Entries => entries.AsReadOnly();
    public int DailyGoal => dailyGoal;

    public WaterEntry AddEntry(int amount, string note = "")
    {
        if (amount <= 0) throw new ArgumentException("Объём должен быть положительным");
        var entry = new WaterEntry(
            nextId,
            DateTime.Now.ToString("o"),
            amount,
            note ?? ""
        );
        entries.Add(entry);
        nextId++;
        return entry;
    }

    public WaterEntry? FindEntry(int id) => entries.FirstOrDefault(e => e.Id == id);

    public bool DeleteEntry(int id) => entries.RemoveAll(e => e.Id == id) > 0;

    public List<WaterEntry> GetTodayEntries()
    {
        var today = DateTime.Today;
        return entries.Where(e => DateTime.Parse(e.Timestamp).Date == today).ToList();
    }

    public List<WaterEntry> GetWeekEntries()
    {
        var weekAgo = DateTime.Now.AddDays(-7);
        return entries.Where(e => DateTime.Parse(e.Timestamp) >= weekAgo).ToList();
    }

    public Dictionary<string, object> GetStats()
    {
        var today = GetTodayEntries();
        var week = GetWeekEntries();
        int todayTotal = today.Sum(e => e.Amount);
        int weekTotal = week.Sum(e => e.Amount);
        double avgDaily = weekTotal / 7.0;
        double progress = dailyGoal > 0 ? (double)todayTotal / dailyGoal * 100 : 0;
        if (progress > 100) progress = 100;
        return new Dictionary<string, object>
        {
            ["today_total"] = todayTotal,
            ["today_count"] = today.Count,
            ["week_total"] = weekTotal,
            ["week_count"] = week.Count,
            ["avg_daily"] = avgDaily,
            ["daily_goal"] = dailyGoal,
            ["progress"] = progress
        };
    }

    public void SetDailyGoal(int goal)
    {
        if (goal <= 0) throw new ArgumentException("Цель должна быть положительной");
        dailyGoal = goal;
    }

    public void SaveToFile(string filename)
    {
        var data = new WaterData { DailyGoal = dailyGoal, Entries = entries };
        var options = new JsonSerializerOptions { WriteIndented = true };
        string json = JsonSerializer.Serialize(data, options);
        File.WriteAllText(filename, json);
    }

    public void LoadFromFile(string filename)
    {
        if (!File.Exists(filename)) return;
        string json = File.ReadAllText(filename);
        var data = JsonSerializer.Deserialize<WaterData>(json);
        if (data != null)
        {
            dailyGoal = data.DailyGoal;
            entries = data.Entries;
            nextId = entries.Any() ? entries.Max(e => e.Id) + 1 : 1;
        }
    }
}

public static class Program
{
    private static string ReadString(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private static int ReadInt(string prompt)
    {
        while (true)
        {
            Console.Write(prompt);
            if (int.TryParse(Console.ReadLine(), out int result))
                return result;
            Console.WriteLine("Введите число.");
        }
    }

    private static void PrintEntry(WaterEntry entry)
    {
        Console.WriteLine($"#{entry.Id} - {entry.Timestamp} - {entry.Amount} мл");
        if (!string.IsNullOrEmpty(entry.Note))
            Console.WriteLine($"   Заметка: {entry.Note}");
    }

    public static void Main()
    {
        var tracker = new WaterTracker();
        try { tracker.LoadFromFile("water_data.json"); }
        catch { Console.WriteLine("Не удалось загрузить данные."); }

        while (true)
        {
            Console.WriteLine("\n===== ТРЕКЕР ВОДНОГО БАЛАНСА (C#) =====");
            Console.WriteLine("1. Добавить запись");
            Console.WriteLine("2. Просмотреть записи за сегодня");
            Console.WriteLine("3. Просмотреть записи за неделю");
            Console.WriteLine("4. Показать статистику");
            Console.WriteLine("5. Установить дневную цель");
            Console.WriteLine("6. Удалить запись");
            Console.WriteLine("7. Сохранить в файл");
            Console.WriteLine("8. Загрузить из файла");
            Console.WriteLine("0. Выход");
            string choice = ReadString("Выберите действие: ");

            switch (choice)
            {
                case "0": return;
                case "1":
                    int amount = ReadInt("Объём (мл): ");
                    if (amount <= 0) { Console.WriteLine("Объём должен быть положительным."); continue; }
                    string note = ReadString("Заметка (необязательно): ");
                    var entry = tracker.AddEntry(amount, note);
                    Console.WriteLine($"Запись добавлена с ID {entry.Id}");
                    break;
                case "2":
                    var today = tracker.GetTodayEntries();
                    if (today.Count == 0) Console.WriteLine("За сегодня записей нет.");
                    else { Console.WriteLine($"\nЗаписи за сегодня ({today.Count}):"); today.ForEach(PrintEntry); }
                    break;
                case "3":
                    var week = tracker.GetWeekEntries();
                    if (week.Count == 0) Console.WriteLine("За неделю записей нет.");
                    else { Console.WriteLine($"\nЗаписи за последние 7 дней ({week.Count}):"); week.ForEach(PrintEntry); }
                    break;
                case "4":
                    var stats = tracker.GetStats();
                    Console.WriteLine("\n=== СТАТИСТИКА ===");
                    Console.WriteLine($"Дневная цель: {stats["daily_goal"]} мл");
                    Console.WriteLine($"Сегодня выпито: {stats["today_total"]} мл ({stats["today_count"]} записей)");
                    Console.WriteLine($"Прогресс: {stats["progress"]:F1}%");
                    Console.WriteLine($"За неделю: {stats["week_total"]} мл ({stats["week_count"]} записей)");
                    Console.WriteLine($"Среднее за день (за неделю): {stats["avg_daily"]:F1} мл");
                    break;
                case "5":
                    int goal = ReadInt("Новая дневная цель (мл): ");
                    try { tracker.SetDailyGoal(goal); Console.WriteLine($"Цель установлена: {goal} мл"); }
                    catch (Exception ex) { Console.WriteLine(ex.Message); }
                    break;
                case "6":
                    int id = ReadInt("Введите ID записи для удаления: ");
                    if (tracker.DeleteEntry(id)) Console.WriteLine("Запись удалена.");
                    else Console.WriteLine("Запись не найдена.");
                    break;
                case "7":
                    try { tracker.SaveToFile("water_data.json"); Console.WriteLine("Сохранено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "8":
                    try { tracker.LoadFromFile("water_data.json"); Console.WriteLine("Загружено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                default: Console.WriteLine("Неизвестная команда."); break;
            }
        }
    }
}
