// WaterTracker.java
import java.io.*;
import java.nio.file.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.stream.Collectors;

record WaterEntry(int id, String timestamp, int amount, String note) implements Serializable {}

class WaterData implements Serializable {
    private static final long serialVersionUID = 1L;
    public int dailyGoal;
    public List<WaterEntry> entries;
}

class WaterTracker implements Serializable {
    private static final long serialVersionUID = 1L;
    private List<WaterEntry> entries = new ArrayList<>();
    private int nextId = 1;
    private int dailyGoal = 2000;

    public WaterEntry addEntry(int amount, String note) {
        if (amount <= 0) throw new IllegalArgumentException("Объём должен быть положительным");
        WaterEntry entry = new WaterEntry(
            nextId,
            LocalDateTime.now().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME),
            amount,
            note != null ? note : ""
        );
        entries.add(entry);
        nextId++;
        return entry;
    }

    public Optional<WaterEntry> findEntry(int id) {
        return entries.stream().filter(e -> e.id() == id).findFirst();
    }

    public boolean deleteEntry(int id) {
        return entries.removeIf(e -> e.id() == id);
    }

    public List<WaterEntry> getTodayEntries() {
        LocalDateTime today = LocalDateTime.now().withHour(0).withMinute(0).withSecond(0).withNano(0);
        return entries.stream()
            .filter(e -> LocalDateTime.parse(e.timestamp()).isAfter(today) || LocalDateTime.parse(e.timestamp()).isEqual(today))
            .collect(Collectors.toList());
    }

    public List<WaterEntry> getWeekEntries() {
        LocalDateTime weekAgo = LocalDateTime.now().minusDays(7);
        return entries.stream()
            .filter(e -> LocalDateTime.parse(e.timestamp()).isAfter(weekAgo) || LocalDateTime.parse(e.timestamp()).isEqual(weekAgo))
            .collect(Collectors.toList());
    }

    public Map<String, Object> getStats() {
        List<WaterEntry> today = getTodayEntries();
        List<WaterEntry> week = getWeekEntries();
        int todayTotal = today.stream().mapToInt(WaterEntry::amount).sum();
        int weekTotal = week.stream().mapToInt(WaterEntry::amount).sum();
        double avgDaily = weekTotal / 7.0;
        double progress = dailyGoal > 0 ? (double) todayTotal / dailyGoal * 100 : 0;
        if (progress > 100) progress = 100;
        Map<String, Object> stats = new HashMap<>();
        stats.put("today_total", todayTotal);
        stats.put("today_count", today.size());
        stats.put("week_total", weekTotal);
        stats.put("week_count", week.size());
        stats.put("avg_daily", avgDaily);
        stats.put("daily_goal", dailyGoal);
        stats.put("progress", progress);
        return stats;
    }

    public void setDailyGoal(int goal) {
        if (goal <= 0) throw new IllegalArgumentException("Цель должна быть положительной");
        dailyGoal = goal;
    }

    public void saveToFile(String filename) throws IOException {
        WaterData data = new WaterData();
        data.dailyGoal = dailyGoal;
        data.entries = new ArrayList<>(entries);
        try (ObjectOutputStream oos = new ObjectOutputStream(Files.newOutputStream(Paths.get(filename)))) {
            oos.writeObject(data);
        }
    }

    public void loadFromFile(String filename) throws IOException, ClassNotFoundException {
        try (ObjectInputStream ois = new ObjectInputStream(Files.newInputStream(Paths.get(filename)))) {
            WaterData data = (WaterData) ois.readObject();
            dailyGoal = data.dailyGoal;
            entries = new ArrayList<>(data.entries);
            for (WaterEntry e : entries) {
                if (e.id() >= nextId) nextId = e.id() + 1;
            }
        }
    }

    public List<WaterEntry> getEntries() { return Collections.unmodifiableList(entries); }
}

public class WaterTrackerApp {
    private static final Scanner scanner = new Scanner(System.in);

    private static String readString(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private static int readInt(String prompt) {
        while (true) {
            try {
                System.out.print(prompt);
                return Integer.parseInt(scanner.nextLine().trim());
            } catch (NumberFormatException e) {
                System.out.println("Введите число.");
            }
        }
    }

    private static void printEntry(WaterEntry entry) {
        System.out.printf("#%d - %s - %d мл%n", entry.id(), entry.timestamp(), entry.amount());
        if (!entry.note().isEmpty()) {
            System.out.printf("   Заметка: %s%n", entry.note());
        }
    }

    public static void main(String[] args) {
        WaterTracker tracker = new WaterTracker();
        try {
            tracker.loadFromFile("water_data.ser");
        } catch (IOException | ClassNotFoundException e) {
            System.out.println("Не удалось загрузить данные, начинаем с пустого трекера.");
        }

        while (true) {
            System.out.println("\n===== ТРЕКЕР ВОДНОГО БАЛАНСА (Java) =====");
            System.out.println("1. Добавить запись");
            System.out.println("2. Просмотреть записи за сегодня");
            System.out.println("3. Просмотреть записи за неделю");
            System.out.println("4. Показать статистику");
            System.out.println("5. Установить дневную цель");
            System.out.println("6. Удалить запись");
            System.out.println("7. Сохранить в файл");
            System.out.println("8. Загрузить из файла");
            System.out.println("0. Выход");
            String choice = readString("Выберите действие: ");

            switch (choice) {
                case "0" -> { return; }
                case "1" -> {
                    int amount = readInt("Объём (мл): ");
                    if (amount <= 0) {
                        System.out.println("Объём должен быть положительным.");
                        continue;
                    }
                    String note = readString("Заметка (необязательно): ");
                    WaterEntry entry = tracker.addEntry(amount, note);
                    System.out.printf("Запись добавлена с ID %d%n", entry.id());
                }
                case "2" -> {
                    var entries = tracker.getTodayEntries();
                    if (entries.isEmpty()) {
                        System.out.println("За сегодня записей нет.");
                    } else {
                        System.out.printf("\nЗаписи за сегодня (%d):%n", entries.size());
                        entries.forEach(WaterTrackerApp::printEntry);
                    }
                }
                case "3" -> {
                    var entries = tracker.getWeekEntries();
                    if (entries.isEmpty()) {
                        System.out.println("За неделю записей нет.");
                    } else {
                        System.out.printf("\nЗаписи за последние 7 дней (%d):%n", entries.size());
                        entries.forEach(WaterTrackerApp::printEntry);
                    }
                }
                case "4" -> {
                    var stats = tracker.getStats();
                    System.out.println("\n=== СТАТИСТИКА ===");
                    System.out.printf("Дневная цель: %d мл%n", stats.get("daily_goal"));
                    System.out.printf("Сегодня выпито: %d мл (%d записей)%n", stats.get("today_total"), stats.get("today_count"));
                    System.out.printf("Прогресс: %.1f%%%n", stats.get("progress"));
                    System.out.printf("За неделю: %d мл (%d записей)%n", stats.get("week_total"), stats.get("week_count"));
                    System.out.printf("Среднее за день (за неделю): %.1f мл%n", stats.get("avg_daily"));
                }
                case "5" -> {
                    int goal = readInt("Новая дневная цель (мл): ");
                    try {
                        tracker.setDailyGoal(goal);
                        System.out.printf("Цель установлена: %d мл%n", goal);
                    } catch (IllegalArgumentException e) {
                        System.out.println(e.getMessage());
                    }
                }
                case "6" -> {
                    int id = readInt("Введите ID записи для удаления: ");
                    if (tracker.deleteEntry(id)) {
                        System.out.println("Запись удалена.");
                    } else {
                        System.out.println("Запись не найдена.");
                    }
                }
                case "7" -> {
                    try {
                        tracker.saveToFile("water_data.ser");
                        System.out.println("Сохранено.");
                    } catch (IOException e) {
                        System.out.println("Ошибка сохранения: " + e.getMessage());
                    }
                }
                case "8" -> {
                    try {
                        tracker.loadFromFile("water_data.ser");
                        System.out.println("Загружено.");
                    } catch (IOException | ClassNotFoundException e) {
                        System.out.println("Ошибка загрузки: " + e.getMessage());
                    }
                }
                default -> System.out.println("Неизвестная команда.");
            }
        }
    }
}
