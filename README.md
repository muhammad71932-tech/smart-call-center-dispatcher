# 📞 Smart Call Center Dispatcher

A professional, full-stack **Call Center Management System** built entirely in **C** with a modern web frontend. Features real-time call dispatching, agent management, priority queuing, and performance analytics — all served from a single lightweight binary.

---

## 🖥️ Preview

| Dashboard | Agents | Dispatch |
|-----------|--------|----------|
| Live stats, queue overview, metrics | Add/edit agents, skill tags, status | Skills-based routing, auto-dispatch |

---

## ✨ Features

### 🧠 Smart Dispatch Engine
- **Skills-based routing** — matches calls to agents by category (billing, tech, sales, etc.)
- **Priority queue** — Emergency → VIP → Normal, with FIFO within each tier
- **Auto-dispatch** — one click assigns all waiting calls to available agents
- **Manual dispatch** — select specific call → agent pairs
- **Load balancing** — prefers agents with fewer handled calls

### 👤 Agent Management
- Add, edit, delete agents
- Real-time status: `AVAILABLE` / `BUSY` / `BREAK` / `OFFLINE`
- Skill tags per agent (multi-skill support)
- Per-agent call statistics

### 📋 Call Management
- Incoming call logging with caller ID, name, priority, category, notes
- Full call lifecycle: `WAITING → ACTIVE → COMPLETED / ABANDONED`
- Auto-calculated wait time and handle time
- Search and filter by status, priority, caller

### 📊 Reports & Analytics
- Daily call volume, resolution rate, abandon rate
- Average wait time and handle time
- Per-agent performance breakdown
- Live dashboard with real-time metrics

### 🌐 Web Frontend
- Clean **light theme** responsive UI
- Sidebar navigation: Dashboard, Agents, Call History, Queue, Dispatch, Reports
- Auto-refreshes every 15 seconds
- Works in any modern browser

---

## 🏗️ Architecture

```
Browser (HTML + CSS + JS)
        ↕  HTTP REST API (JSON)
C Backend (Mongoose HTTP Server)
        ↕
SQLite3 Database
```

### Project Structure

```
smart-call-center-dispatcher/
├── backend/
│   ├── main.c              ← HTTP server entry point
│   ├── mongoose.c/h        ← Embedded HTTP library
│   ├── Makefile
│   └── src/
│       ├── db.c/h          ← SQLite connection & schema
│       ├── agent.c/h       ← Agent CRUD
│       ├── call.c/h        ← Call lifecycle management
│       ├── dispatcher.c/h  ← Smart routing engine
│       ├── report.c/h      ← Analytics queries
│       └── routes.c/h      ← REST API endpoints
├── frontend/
│   ├── index.html
│   ├── css/style.css       ← Light theme UI
│   └── js/app.js           ← SPA logic
└── start.sh                ← One-command launcher
```

---

## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu / Debian / Parrot OS
sudo apt install gcc make libsqlite3-dev

# macOS
brew install sqlite3

# Windows
# Use MSYS2: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sqlite3
```

### Run

```bash
git clone https://github.com/muhammad71932-tech/smart-call-center-dispatcher.git
cd smart-call-center-dispatcher
./start.sh
```

Then open **http://localhost:9090** in your browser.

> `start.sh` automatically builds the project and kills any previous instance on port 9090.

---

## 📡 REST API

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/agents` | List all agents |
| `POST` | `/api/agents` | Create agent |
| `PUT` | `/api/agents/:id` | Update agent |
| `DELETE` | `/api/agents/:id` | Delete agent |
| `PUT` | `/api/agents/status/:id` | Update agent status |
| `GET` | `/api/calls` | Call history |
| `POST` | `/api/calls` | Log new incoming call |
| `PUT` | `/api/calls/resolve/:id` | Mark call as completed |
| `PUT` | `/api/calls/abandon/:id` | Mark call as abandoned |
| `GET` | `/api/queue` | Live call queue (priority sorted) |
| `POST` | `/api/dispatch` | Dispatch a call to an agent |
| `POST` | `/api/dispatch/auto` | Auto-dispatch all waiting calls |
| `GET` | `/api/reports/dashboard` | Live dashboard stats |
| `GET` | `/api/reports/daily` | Daily call statistics |
| `GET` | `/api/reports/agents` | Per-agent performance |

---

## 🗄️ Database Schema

```sql
agents (id, name, phone, email, status, skills, calls_handled, created_at)
calls  (id, caller_id, caller_name, priority, category, status,
        agent_id, notes, wait_time, handle_time, created_at, resolved_at)
```

Data is stored in `backend/data/callcenter.db` — a single portable SQLite file.

---

## 🔧 Tech Stack

| Layer | Technology |
|-------|-----------|
| Backend | C (C11) |
| HTTP Server | [Mongoose](https://github.com/cesanta/mongoose) (embedded) |
| Database | SQLite3 |
| Frontend | HTML5 + CSS3 + Vanilla JavaScript |
| Build | GCC + Make |

---

## 📦 Portability

The compiled binary + `frontend/` folder + `data/callcenter.db` is everything you need. Copy to any machine, rebuild with `make`, and run.

---

## 📄 License

MIT License — free to use, modify, and distribute.

---

*Built with C, Mongoose, SQLite3, and a clean web frontend.*
