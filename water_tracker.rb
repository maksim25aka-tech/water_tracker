# water_tracker.rb
require 'json'
require 'date'

class WaterEntry
  attr_accessor :id, :timestamp, :amount, :note

  def initialize(id, amount, note = "", timestamp = DateTime.now.iso8601)
    @id = id
    @timestamp = timestamp
    @amount = amount
    @note = note
  end

  def to_h
    { id: @id, timestamp: @timestamp, amount: @amount, note: @note }
  end

  def self.from_h(hash)
    WaterEntry.new(hash[:id], hash[:amount], hash[:note], hash[:timestamp])
  end
end

class WaterTracker
  attr_reader :entries, :daily_goal

  def initialize
    @entries = []
    @next_id = 1
    @daily_goal = 2000
  end

  def add_entry(amount, note = "")
    raise ArgumentError, "Объём должен быть положительным" if amount <= 0
    entry = WaterEntry.new(@next_id, amount, note)
    @entries << entry
    @next_id += 1
    entry
  end

  def find_entry(id)
    @entries.find { |e| e.id == id }
  end

  def delete_entry(id)
    entry = find_entry(id)
    return false unless entry
    @entries.delete(entry)
    true
  end

  def today_entries
    today = Date.today
    @entries.select { |e| Date.parse(e.timestamp) == today }
  end

  def week_entries
    week_ago = DateTime.now - 7
    @entries.select { |e| DateTime.parse(e.timestamp) >= week_ago }
  end

  def stats
    today = today_entries
    week = week_entries
    today_total = today.sum(&:amount)
    week_total = week.sum(&:amount)
    week_days = 7
    avg_daily = week_total / week_days.to_f
    progress = @daily_goal > 0 ? (today_total.to_f / @daily_goal * 100) : 0
    {
      today_total: today_total,
      today_count: today.size,
      week_total: week_total,
      week_count: week.size,
      avg_daily: avg_daily,
      daily_goal: @daily_goal,
      progress: [progress, 100].min
    }
  end

  def daily_goal=(value)
    raise ArgumentError, "Цель должна быть положительной" if value <= 0
    @daily_goal = value
  end

  def save_to_file(filename = "water_data.json")
    data = {
      daily_goal: @daily_goal,
      entries: @entries.map(&:to_h)
    }
    File.write(filename, JSON.pretty_generate(data))
  end

  def load_from_file(filename = "water_data.json")
    return unless File.exist?(filename)
    data = JSON.parse(File.read(filename), symbolize_names: true)
    @daily_goal = data[:daily_goal] || 2000
    @entries.clear
    data[:entries].each do |item|
      entry = WaterEntry.from_h(item)
      @entries << entry
      @next_id = entry.id + 1 if entry.id >= @next_id
    end
  rescue JSON::ParserError
    puts "Ошибка чтения файла."
  end
end

def print_entry(entry)
  puts "##{entry.id} - #{entry.timestamp} - #{entry.amount} мл"
  puts "   Заметка: #{entry.note}" unless entry.note.empty?
end

def main
  tracker = WaterTracker.new
  tracker.load_from_file

  loop do
    puts "\n===== ТРЕКЕР ВОДНОГО БАЛАНСА (Ruby) ====="
    puts "1. Добавить запись"
    puts "2. Просмотреть записи за сегодня"
    puts "3. Просмотреть записи за неделю"
    puts "4. Показать статистику"
    puts "5. Установить дневную цель"
    puts "6. Удалить запись"
    puts "7. Сохранить в файл"
    puts "8. Загрузить из файла"
    puts "0. Выход"
    print "Выберите действие: "
    choice = gets.chomp

    case choice
    when "0"
      break
    when "1"
      print "Объём (мл): "
      amount = gets.chomp.to_i
      if amount <= 0
        puts "Объём должен быть положительным."
        next
      end
      print "Заметка (необязательно): "
      note = gets.chomp
      entry = tracker.add_entry(amount, note)
      puts "Запись добавлена с ID #{entry.id}"
    when "2"
      entries = tracker.today_entries
      if entries.empty?
        puts "За сегодня записей нет."
      else
        puts "\nЗаписи за сегодня (#{entries.size}):"
        entries.each { |e| print_entry(e) }
      end
    when "3"
      entries = tracker.week_entries
      if entries.empty?
        puts "За неделю записей нет."
      else
        puts "\nЗаписи за последние 7 дней (#{entries.size}):"
        entries.each { |e| print_entry(e) }
      end
    when "4"
      stats = tracker.stats
      puts "\n=== СТАТИСТИКА ==="
      puts "Дневная цель: #{stats[:daily_goal]} мл"
      puts "Сегодня выпито: #{stats[:today_total]} мл (#{stats[:today_count]} записей)"
      puts "Прогресс: #{'%.1f' % stats[:progress]}%"
      puts "За неделю: #{stats[:week_total]} мл (#{stats[:week_count]} записей)"
      puts "Среднее за день (за неделю): #{'%.1f' % stats[:avg_daily]} мл"
    when "5"
      print "Новая дневная цель (мл): "
      new_goal = gets.chomp.to_i
      if new_goal <= 0
        puts "Цель должна быть положительной."
        next
      end
      tracker.daily_goal = new_goal
      puts "Цель установлена: #{new_goal} мл"
    when "6"
      print "Введите ID записи для удаления: "
      id = gets.chomp.to_i
      if tracker.delete_entry(id)
        puts "Запись удалена."
      else
        puts "Запись не найдена."
      end
    when "7"
      tracker.save_to_file
      puts "Сохранено."
    when "8"
      tracker.load_from_file
      puts "Загружено."
    else
      puts "Неизвестная команда."
    end
  end
end

main if __FILE__ == $0
