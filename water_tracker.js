// water_tracker.js
const fs = require('fs').promises;
const readline = require('readline');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

const question = (prompt) => new Promise(resolve => rl.question(prompt, resolve));

class WaterEntry {
    constructor(id, amount, note = '', timestamp = new Date().toISOString()) {
        this.id = id;
        this.timestamp = timestamp;
        this.amount = amount;
        this.note = note;
    }
}

class WaterTracker {
    constructor() {
        this.entries = [];
        this.nextId = 1;
        this.dailyGoal = 2000;
    }

    addEntry(amount, note = '') {
        if (amount <= 0) throw new Error('Объём должен быть положительным');
        const entry = new WaterEntry(this.nextId, amount, note);
        this.entries.push(entry);
        this.nextId++;
        return entry;
    }

    findEntry(id) {
        return this.entries.find(e => e.id === id);
    }

    deleteEntry(id) {
        const index = this.entries.findIndex(e => e.id === id);
        if (index === -1) return false;
        this.entries.splice(index, 1);
        return true;
    }

    getTodayEntries() {
        const today = new Date();
        today.setHours(0, 0, 0, 0);
        return this.entries.filter(e => new Date(e.timestamp) >= today);
    }

    getWeekEntries() {
        const weekAgo = new Date();
        weekAgo.setDate(weekAgo.getDate() - 7);
        return this.entries.filter(e => new Date(e.timestamp) >= weekAgo);
    }

    getStats() {
        const today = this.getTodayEntries();
        const week = this.getWeekEntries();
        const todayTotal = today.reduce((sum, e) => sum + e.amount, 0);
        const weekTotal = week.reduce((sum, e) => sum + e.amount, 0);
        const avgDaily = weekTotal / 7;
        let progress = this.dailyGoal > 0 ? (todayTotal / this.dailyGoal) * 100 : 0;
        if (progress > 100) progress = 100;
        return {
            todayTotal,
            todayCount: today.length,
            weekTotal,
            weekCount: week.length,
            avgDaily,
            dailyGoal: this.dailyGoal,
            progress
        };
    }

    setDailyGoal(goal) {
        if (goal <= 0) throw new Error('Цель должна быть положительной');
        this.dailyGoal = goal;
    }

    async saveToFile(filename = 'water_data.json') {
        const data = {
            dailyGoal: this.dailyGoal,
            entries: this.entries
        };
        await fs.writeFile(filename, JSON.stringify(data, null, 2));
    }

    async loadFromFile(filename = 'water_data.json') {
        try {
            const data = await fs.readFile(filename, 'utf8');
            const parsed = JSON.parse(data);
            this.dailyGoal = parsed.dailyGoal || 2000;
            this.entries = parsed.entries.map(e => Object.assign(new WaterEntry(0), e));
            this.nextId = this.entries.reduce((max, e) => Math.max(max, e.id), 0) + 1;
        } catch (err) {
            if (err.code !== 'ENOENT') throw err;
        }
    }
}

function printEntry(entry) {
    console.log(`#${entry.id} - ${entry.timestamp} - ${entry.amount} мл`);
    if (entry.note) console.log(`   Заметка: ${entry.note}`);
}

async function main() {
    const tracker = new WaterTracker();
    await tracker.loadFromFile();

    while (true) {
        console.log('\n===== ТРЕКЕР ВОДНОГО БАЛАНСА (JavaScript) =====');
        console.log('1. Добавить запись');
        console.log('2. Просмотреть записи за сегодня');
        console.log('3. Просмотреть записи за неделю');
        console.log('4. Показать статистику');
        console.log('5. Установить дневную цель');
        console.log('6. Удалить запись');
        console.log('7. Сохранить в файл');
        console.log('8. Загрузить из файла');
        console.log('0. Выход');
        const choice = await question('Выберите действие: ');

        if (choice === '0') break;

        switch (choice) {
            case '1': {
                const amount = parseInt(await question('Объём (мл): '));
                if (isNaN(amount) || amount <= 0) {
                    console.log('Объём должен быть положительным числом.');
                    continue;
                }
                const note = await question('Заметка (необязательно): ');
                try {
                    const entry = tracker.addEntry(amount, note);
                    console.log(`Запись добавлена с ID ${entry.id}`);
                } catch (err) {
                    console.log(err.message);
                }
                break;
            }
            case '2': {
                const entries = tracker.getTodayEntries();
                if (entries.length === 0) {
                    console.log('За сегодня записей нет.');
                } else {
                    console.log(`\nЗаписи за сегодня (${entries.length}):`);
                    entries.forEach(printEntry);
                }
                break;
            }
            case '3': {
                const entries = tracker.getWeekEntries();
                if (entries.length === 0) {
                    console.log('За неделю записей нет.');
                } else {
                    console.log(`\nЗаписи за последние 7 дней (${entries.length}):`);
                    entries.forEach(printEntry);
                }
                break;
            }
            case '4': {
                const stats = tracker.getStats();
                console.log('\n=== СТАТИСТИКА ===');
                console.log(`Дневная цель: ${stats.dailyGoal} мл`);
                console.log(`Сегодня выпито: ${stats.todayTotal} мл (${stats.todayCount} записей)`);
                console.log(`Прогресс: ${stats.progress.toFixed(1)}%`);
                console.log(`За неделю: ${stats.weekTotal} мл (${stats.weekCount} записей)`);
                console.log(`Среднее за день (за неделю): ${stats.avgDaily.toFixed(1)} мл`);
                break;
            }
            case '5': {
                const goal = parseInt(await question('Новая дневная цель (мл): '));
                if (isNaN(goal) || goal <= 0) {
                    console.log('Цель должна быть положительным числом.');
                    continue;
                }
                try {
                    tracker.setDailyGoal(goal);
                    console.log(`Цель установлена: ${goal} мл`);
                } catch (err) {
                    console.log(err.message);
                }
                break;
            }
            case '6': {
                const id = parseInt(await question('Введите ID записи для удаления: '));
                if (tracker.deleteEntry(id)) {
                    console.log('Запись удалена.');
                } else {
                    console.log('Запись не найдена.');
                }
                break;
            }
            case '7':
                try {
                    await tracker.saveToFile();
                    console.log('Сохранено.');
                } catch (err) {
                    console.log('Ошибка сохранения:', err.message);
                }
                break;
            case '8':
                try {
                    await tracker.loadFromFile();
                    console.log('Загружено.');
                } catch (err) {
                    console.log('Ошибка загрузки:', err.message);
                }
                break;
            default:
                console.log('Неизвестная команда.');
        }
    }
    rl.close();
}

main().catch(console.error);
