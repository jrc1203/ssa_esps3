// =============================================================
//  html_dashboard.h — Game Dashboard (served at /)
//  Dark theme, gold accents, live WebSocket updates
// =============================================================
#ifndef HTML_DASHBOARD_H
#define HTML_DASHBOARD_H

const char PAGE_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>House of Ancient Secrets — Dashboard</title>
<link href="https://fonts.googleapis.com/css2?family=Cinzel:wght@400;700;900&family=Inter:wght@300;400;600;700&display=swap" rel="stylesheet">
<style>
*{margin:0;padding:0;box-sizing:border-box;}
:root{
  --bg:#0a0f14;--bg2:#111820;--bg3:#1a2230;
  --gold:#d4af37;--gold-dim:#a6882a;
  --burgundy:#800020;--burgundy-glow:#a0003a;
  --green:#10b981;--green-dim:#0d9668;
  --white:#ffffff;--muted:#a8a29e;--muted2:#6b6560;
  --card-bg:rgba(26,34,48,0.85);
  --border:rgba(212,175,55,0.2);
}
body{
  font-family:'Inter',sans-serif;background:var(--bg);color:var(--white);
  min-height:100vh;overflow-x:hidden;
  background-image:radial-gradient(ellipse at 20% 50%,rgba(128,0,32,0.08) 0%,transparent 50%),
                   radial-gradient(ellipse at 80% 20%,rgba(212,175,55,0.05) 0%,transparent 50%);
}
/* ── Header ── */
.header{
  display:flex;justify-content:space-between;align-items:center;
  padding:20px 30px;border-bottom:1px solid var(--border);
  background:rgba(10,15,20,0.9);backdrop-filter:blur(10px);
  position:sticky;top:0;z-index:100;
}
.logo{display:flex;align-items:center;gap:14px;}
.logo-icon{font-size:28px;}
.logo h1{
  font-family:'Cinzel',serif;font-size:20px;font-weight:700;
  color:var(--gold);letter-spacing:1.5px;
}
.logo .subtitle{font-size:11px;color:var(--muted);letter-spacing:2px;text-transform:uppercase;}
.header-right{display:flex;align-items:center;gap:16px;}
.nav-links{display:flex;gap:8px;}
.nav-link{
  font-size:12px;color:var(--muted);text-decoration:none;
  padding:6px 14px;border-radius:8px;letter-spacing:0.5px;
  border:1px solid var(--border);transition:all .3s;
  display:flex;align-items:center;gap:5px;
}
.nav-link:hover{color:var(--gold);border-color:var(--gold-dim);background:rgba(212,175,55,0.06);}
.conn-status{display:flex;align-items:center;gap:8px;font-size:13px;color:var(--muted);}
.conn-dot{width:8px;height:8px;border-radius:50%;background:#555;transition:background .3s;}
.conn-dot.online{background:var(--green);box-shadow:0 0 8px var(--green);}

/* ── Main Container ── */
.container{max-width:1100px;margin:0 auto;padding:24px 20px;}

/* ── Game State Banner ── */
.state-banner{
  text-align:center;padding:10px;margin-bottom:20px;
  border-radius:10px;font-size:14px;font-weight:600;
  letter-spacing:2px;text-transform:uppercase;
  border:1px solid var(--border);background:var(--card-bg);
}
.state-banner.idle{color:var(--muted);}
.state-banner.active{color:var(--green);border-color:rgba(16,185,129,0.3);
  background:rgba(16,185,129,0.05);}
.state-banner.quiz{color:var(--gold);border-color:rgba(212,175,55,0.3);
  background:rgba(212,175,55,0.05);animation:pulse 2s infinite;}
.state-banner.over{color:var(--burgundy-glow);border-color:rgba(160,0,58,0.3);
  background:rgba(128,0,32,0.08);}
@keyframes pulse{0%,100%{opacity:1;}50%{opacity:.7;}}

/* ── Timer ── */
.timer-section{text-align:center;margin:20px 0 28px;}
.timer{
  font-family:'Cinzel',serif;font-size:64px;font-weight:900;
  color:var(--gold);letter-spacing:4px;
  text-shadow:0 0 30px rgba(212,175,55,0.3);
}
.timer.warning{color:#ef4444;text-shadow:0 0 30px rgba(239,68,68,0.4);animation:pulse 1s infinite;}
.timer-label{font-size:12px;color:var(--muted);letter-spacing:3px;text-transform:uppercase;margin-top:4px;}

/* ── Room Cards ── */
.rooms{display:grid;grid-template-columns:repeat(5,1fr);gap:14px;margin-bottom:28px;}
.room-card{
  background:var(--card-bg);border:1px solid var(--border);border-radius:14px;
  padding:18px 10px;text-align:center;transition:all .4s ease;
  position:relative;overflow:hidden;
}
.room-card::before{
  content:'';position:absolute;top:0;left:0;right:0;height:3px;
  background:linear-gradient(90deg,transparent,var(--gold-dim),transparent);
  opacity:0;transition:opacity .4s;
}
.room-card.hit{
  border-color:var(--gold);background:rgba(212,175,55,0.08);
  box-shadow:0 0 20px rgba(212,175,55,0.15);
}
.room-card.hit::before{opacity:1;}
.room-num{
  font-family:'Cinzel',serif;font-size:11px;color:var(--muted2);
  letter-spacing:2px;text-transform:uppercase;margin-bottom:6px;
}
.room-icon{font-size:32px;margin:8px 0;}
.room-name{font-size:11px;color:var(--muted);line-height:1.3;min-height:30px;}
.room-status{
  margin-top:10px;font-size:20px;
  transition:transform .3s;
}
.room-card.hit .room-status{transform:scale(1.2);}
.room-card.hit .room-name{color:var(--gold);}

/* ── Score ── */
.score-section{
  text-align:center;padding:24px;margin-bottom:28px;
  background:var(--card-bg);border:1px solid var(--border);border-radius:14px;
}
.score-label{font-size:12px;color:var(--muted);letter-spacing:3px;text-transform:uppercase;}
.score-value{
  font-family:'Cinzel',serif;font-size:52px;font-weight:900;
  color:var(--white);margin:6px 0;
  transition:color .3s;
}
.score-value.pop{color:var(--gold);transform:scale(1.05);}
.score-breakdown{font-size:12px;color:var(--muted2);margin-top:4px;}

/* ── Quiz Section ── */
.quiz-section{
  display:none;padding:20px;margin-bottom:20px;
  background:var(--card-bg);border:1px solid rgba(212,175,55,0.3);border-radius:14px;
}
.quiz-section.active{display:block;animation:fadeIn .5s;}
.quiz-title{font-family:'Cinzel',serif;color:var(--gold);font-size:16px;margin-bottom:12px;}
.quiz-question{font-size:15px;color:var(--white);margin-bottom:14px;line-height:1.5;}
.quiz-options{display:flex;flex-direction:column;gap:8px;}
.quiz-opt{
  padding:10px 14px;border-radius:8px;font-size:13px;
  background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);
  color:var(--muted);transition:all .3s;
}
.quiz-opt.correct{border-color:var(--green);color:var(--green);background:rgba(16,185,129,0.1);}
.quiz-opt.wrong{border-color:#ef4444;color:#ef4444;background:rgba(239,68,68,0.08);}
.quiz-timer{font-size:13px;color:var(--gold);margin-top:10px;}

/* ── Activity Log ── */
.log-section{margin-bottom:28px;}
.log-title{
  font-size:12px;color:var(--muted);letter-spacing:2px;
  text-transform:uppercase;margin-bottom:10px;
}
.log-list{
  max-height:180px;overflow-y:auto;
  background:var(--card-bg);border:1px solid var(--border);border-radius:10px;
  padding:12px;
}
.log-list::-webkit-scrollbar{width:4px;}
.log-list::-webkit-scrollbar-thumb{background:var(--muted2);border-radius:2px;}
.log-entry{
  font-size:12px;color:var(--muted);padding:6px 0;
  border-bottom:1px solid rgba(255,255,255,0.03);
  animation:fadeIn .3s;
}
.log-entry:last-child{border-bottom:none;}
.log-entry .time{color:var(--muted2);margin-right:8px;}
.log-entry .text{color:var(--white);}
.log-entry .pts{color:var(--gold);font-weight:600;}
@keyframes fadeIn{from{opacity:0;transform:translateY(-5px);}to{opacity:1;transform:translateY(0);}}

/* ── Controls ── */
.controls{display:flex;gap:12px;justify-content:center;flex-wrap:wrap;}
.btn{
  padding:12px 28px;border:none;border-radius:10px;
  font-family:'Inter',sans-serif;font-size:14px;font-weight:600;
  cursor:pointer;letter-spacing:1px;transition:all .3s;
  display:flex;align-items:center;gap:8px;
}
.btn:active{transform:scale(0.97);}
.btn-start{background:var(--green);color:#fff;}
.btn-start:hover{background:#0d9668;box-shadow:0 0 20px rgba(16,185,129,0.3);}
.btn-end{background:var(--burgundy);color:#fff;}
.btn-end:hover{background:var(--burgundy-glow);box-shadow:0 0 20px rgba(128,0,32,0.3);}
.btn-reset{background:var(--bg3);color:var(--muted);border:1px solid var(--border);}
.btn-reset:hover{color:var(--white);border-color:var(--gold-dim);}
.btn:disabled{opacity:0.4;cursor:not-allowed;transform:none!important;}

/* ── Responsive ── */
@media(max-width:700px){
  .rooms{grid-template-columns:repeat(3,1fr);}
  .timer{font-size:48px;}
  .score-value{font-size:40px;}
  .header{padding:14px 16px;}
  .logo h1{font-size:16px;}
}
@media(max-width:480px){
  .rooms{grid-template-columns:repeat(2,1fr);}
  .controls{flex-direction:column;align-items:stretch;}
}
</style>
</head>
<body>

<div class="header">
  <div class="logo">
    <span class="logo-icon">🏛</span>
    <div>
      <h1>HOUSE OF ANCIENT SECRETS</h1>
      <div class="subtitle">India's Spiritual Heritage</div>
    </div>
  </div>
  <div class="header-right">
    <nav class="nav-links">
      <a class="nav-link" href="/hologram">📱 Hologram</a>
      <a class="nav-link" href="/admin">⚙️ Admin</a>
    </nav>
    <div class="conn-status">
      <div class="conn-dot" id="connDot"></div>
      <span id="connText">Connecting...</span>
    </div>
  </div>
</div>

<div class="container">

  <div class="state-banner idle" id="stateBanner">IDLE — WAITING TO START</div>

  <div class="timer-section">
    <div class="timer" id="timerDisplay">5:00</div>
    <div class="timer-label">Time Remaining</div>
  </div>

  <div class="rooms" id="roomGrid">
    <div class="room-card" id="room0">
      <div class="room-num">Room 1</div>
      <div class="room-icon">🛕</div>
      <div class="room-name">Dakshineswar Kali Temple</div>
      <div class="room-status">🔒</div>
    </div>
    <div class="room-card" id="room1">
      <div class="room-num">Room 2</div>
      <div class="room-icon">🙏</div>
      <div class="room-name">Swami Vivekananda</div>
      <div class="room-status">🔒</div>
    </div>
    <div class="room-card" id="room2">
      <div class="room-num">Room 3</div>
      <div class="room-icon">🙏</div>
      <div class="room-name">Ramakrishna Paramahamsa</div>
      <div class="room-status">🔒</div>
    </div>
    <div class="room-card" id="room3">
      <div class="room-num">Room 4</div>
      <div class="room-icon">🙏</div>
      <div class="room-name">Maa Sharada Devi</div>
      <div class="room-status">🔒</div>
    </div>
    <div class="room-card" id="room4">
      <div class="room-num">Room 5</div>
      <div class="room-icon">🛕</div>
      <div class="room-name">Belur Math</div>
      <div class="room-status">🔒</div>
    </div>
  </div>

  <div class="score-section">
    <div class="score-label">Total Score</div>
    <div class="score-value" id="scoreDisplay">0</div>
    <div class="score-breakdown" id="scoreBreakdown">Rooms: 0 | Quiz: 0 | Time Bonus: —</div>
  </div>

  <div class="quiz-section" id="quizSection">
    <div class="quiz-title">📝 QUIZ TIME</div>
    <div class="quiz-question" id="quizQuestion">Loading question...</div>
    <div class="quiz-options" id="quizOptions">
      <div class="quiz-opt" id="quizOptA">A) —</div>
      <div class="quiz-opt" id="quizOptB">B) —</div>
      <div class="quiz-opt" id="quizOptC">C) —</div>
    </div>
    <div class="quiz-timer" id="quizTimer">⏱ Time: --</div>
  </div>

  <div class="log-section">
    <div class="log-title">📋 Activity Log</div>
    <div class="log-list" id="logList">
      <div class="log-entry"><span class="text" style="color:var(--muted2)">Waiting for game to start...</span></div>
    </div>
  </div>

  <div class="controls">
    <button class="btn btn-start" id="btnStart" onclick="sendAction('start')">🟢 Start Game</button>
    <button class="btn btn-end" id="btnEnd" onclick="sendAction('end')" disabled>🔴 End Game</button>
    <button class="btn btn-reset" id="btnReset" onclick="sendAction('reset')">🔄 Reset</button>
  </div>

</div>

<script>
// ── WebSocket Connection ──
let ws;
let reconnectTimer;
let gameTimer = null;
let remainingSeconds = 0;
let roomsHitCount = 0;
let quizPoints = 0;

function connectWS() {
  const host = window.location.hostname;
  const port = window.location.port || '80';
  ws = new WebSocket(`ws://${host}:${port}/ws`);

  ws.onopen = () => {
    document.getElementById('connDot').classList.add('online');
    document.getElementById('connText').textContent = 'Connected';
    addLog('Connected to ESP32', '');
    // Request current state
    ws.send(JSON.stringify({action:'get_status'}));
  };

  ws.onclose = () => {
    document.getElementById('connDot').classList.remove('online');
    document.getElementById('connText').textContent = 'Disconnected';
    clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(connectWS, 3000);
  };

  ws.onerror = () => { ws.close(); };

  ws.onmessage = (e) => {
    try {
      const data = JSON.parse(e.data);
      handleEvent(data);
    } catch(err) {
      console.error('WS parse error:', err);
    }
  };
}

function sendAction(action) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({action: action}));
  }
}

// ── Event Handler ──
function handleEvent(data) {
  switch(data.event) {
    case 'status':
      updateFullState(data);
      break;
    case 'game_start':
      setBanner('active', 'GAME ACTIVE');
      remainingSeconds = data.timer || 300;
      startTimer();
      resetRoomCards();
      document.getElementById('scoreDisplay').textContent = '0';
      roomsHitCount = 0;
      quizPoints = 0;
      updateBreakdown();
      document.getElementById('btnStart').disabled = true;
      document.getElementById('btnEnd').disabled = false;
      addLog('Game Started!', '', 'var(--green)');
      break;

    case 'room_hit':
      const ri = data.room;
      const card = document.getElementById('room' + ri);
      if (card) {
        card.classList.add('hit');
        card.querySelector('.room-status').textContent = '✅';
      }
      roomsHitCount = data.roomsHit || (roomsHitCount + 1);
      animateScore(data.total);
      addLog(`Room ${ri+1} Hit — ${data.name}`, `+${data.pts}`, 'var(--gold)');
      updateBreakdown();
      break;

    case 'belur_rising':
      addLog('Belur Math is Rising!', '', 'var(--gold)');
      break;

    case 'car_detected':
      addLog('Car detected — Table flipping!', '', 'var(--green)');
      break;

    case 'quiz_start':
      setBanner('quiz', 'QUIZ MODE');
      document.getElementById('quizSection').classList.add('active');
      addLog('Quiz Mode activated!', '', 'var(--gold)');
      break;

    case 'quiz_question':
      document.getElementById('quizQuestion').textContent = data.question;
      document.getElementById('quizOptA').textContent = 'A) ' + data.optA;
      document.getElementById('quizOptB').textContent = 'B) ' + data.optB;
      document.getElementById('quizOptC').textContent = 'C) ' + data.optC;
      document.getElementById('quizOptA').className = 'quiz-opt';
      document.getElementById('quizOptB').className = 'quiz-opt';
      document.getElementById('quizOptC').className = 'quiz-opt';
      if (data.quizTime) {
        document.getElementById('quizTimer').textContent = '⏱ Time: ' + data.quizTime + 's';
      }
      break;

    case 'quiz_timer':
      document.getElementById('quizTimer').textContent = '⏱ Time: ' + data.remaining + 's';
      break;

    case 'quiz_answer':
      const opts = ['quizOptA','quizOptB','quizOptC'];
      if (data.correct) {
        opts[data.selected].classList = 'quiz-opt correct';
        quizPoints = data.quizPts || 200;
        addLog('Quiz: Correct!', '+' + quizPoints, 'var(--green)');
      } else {
        opts[data.selected].classList = 'quiz-opt wrong';
        if (data.correctIdx !== undefined) {
          opts[data.correctIdx].classList = 'quiz-opt correct';
        }
        addLog('Quiz: Wrong answer', '', '#ef4444');
      }
      animateScore(data.total);
      updateBreakdown();
      break;

    case 'timer_update':
      remainingSeconds = data.remaining;
      updateTimerDisplay();
      break;

    case 'game_over':
      stopTimer();
      setBanner('over', 'GAME OVER');
      animateScore(data.score);
      document.getElementById('btnStart').disabled = false;
      document.getElementById('btnEnd').disabled = true;
      document.getElementById('quizSection').classList.remove('active');
      addLog(`Game Over! Final Score: ${data.score}`, '', 'var(--burgundy-glow)');
      if (data.timeBonus !== undefined) {
        document.getElementById('scoreBreakdown').textContent =
          `Rooms: ${data.roomPts || 0} | Quiz: ${data.quizPts || 0} | Time Bonus: +${data.timeBonus}`;
      }
      break;

    case 'game_reset':
      stopTimer();
      setBanner('idle', 'IDLE — WAITING TO START');
      resetRoomCards();
      document.getElementById('scoreDisplay').textContent = '0';
      document.getElementById('timerDisplay').textContent = '5:00';
      document.getElementById('timerDisplay').classList.remove('warning');
      document.getElementById('btnStart').disabled = false;
      document.getElementById('btnEnd').disabled = true;
      document.getElementById('quizSection').classList.remove('active');
      roomsHitCount = 0; quizPoints = 0;
      updateBreakdown();
      addLog('System Reset', '', 'var(--muted)');
      break;
  }
}

function updateFullState(data) {
  if (data.state === 'idle') {
    setBanner('idle', 'IDLE — WAITING TO START');
    document.getElementById('btnStart').disabled = false;
    document.getElementById('btnEnd').disabled = true;
  } else if (data.state === 'active' || data.state === 'hit_effect' ||
             data.state === 'belur' || data.state === 'wait_exit' ||
             data.state === 'table_flip') {
    setBanner('active', 'GAME ACTIVE');
    document.getElementById('btnStart').disabled = true;
    document.getElementById('btnEnd').disabled = false;
  } else if (data.state === 'quiz') {
    setBanner('quiz', 'QUIZ MODE');
    document.getElementById('btnStart').disabled = true;
    document.getElementById('btnEnd').disabled = false;
    document.getElementById('quizSection').classList.add('active');
  } else if (data.state === 'over') {
    setBanner('over', 'GAME OVER');
    document.getElementById('btnStart').disabled = false;
    document.getElementById('btnEnd').disabled = true;
  }

  if (data.score !== undefined) {
    document.getElementById('scoreDisplay').textContent = data.score;
  }
  if (data.remaining !== undefined) {
    remainingSeconds = data.remaining;
    updateTimerDisplay();
    if (data.state !== 'idle' && data.state !== 'over') startTimer();
  }
  if (data.roomsHit) {
    data.roomsHit.forEach((hit, i) => {
      if (hit) {
        const card = document.getElementById('room' + i);
        if (card) { card.classList.add('hit'); card.querySelector('.room-status').textContent = '✅'; }
      }
    });
  }
}

// ── UI Helpers ──
function setBanner(cls, text) {
  const b = document.getElementById('stateBanner');
  b.className = 'state-banner ' + cls;
  b.textContent = text;
}

function resetRoomCards() {
  for (let i = 0; i < 5; i++) {
    const card = document.getElementById('room' + i);
    if (card) { card.classList.remove('hit'); card.querySelector('.room-status').textContent = '🔒'; }
  }
}

function animateScore(val) {
  const el = document.getElementById('scoreDisplay');
  el.textContent = val;
  el.classList.add('pop');
  setTimeout(() => el.classList.remove('pop'), 400);
}

function updateBreakdown() {
  const roomPts = roomsHitCount * (window._ptsRoom || 100);
  document.getElementById('scoreBreakdown').textContent =
    `Rooms: ${roomPts} | Quiz: ${quizPoints} | Time Bonus: —`;
}

function addLog(text, pts, color) {
  const list = document.getElementById('logList');
  const now = new Date();
  const ts = now.toLocaleTimeString('en-US',{hour12:false});
  const entry = document.createElement('div');
  entry.className = 'log-entry';
  entry.innerHTML = `<span class="time">${ts}</span><span class="text">${text}</span>` +
    (pts ? ` <span class="pts" style="color:${color||'var(--gold)'}">${pts}</span>` : '');
  list.insertBefore(entry, list.firstChild);
  if (list.children.length > 50) list.removeChild(list.lastChild);
}

// ── Timer ──
function startTimer() {
  stopTimer();
  updateTimerDisplay();
  gameTimer = setInterval(() => {
    if (remainingSeconds > 0) remainingSeconds--;
    updateTimerDisplay();
  }, 1000);
}

function stopTimer() {
  if (gameTimer) { clearInterval(gameTimer); gameTimer = null; }
}

function updateTimerDisplay() {
  const m = Math.floor(remainingSeconds / 60);
  const s = remainingSeconds % 60;
  const el = document.getElementById('timerDisplay');
  el.textContent = `${m}:${s.toString().padStart(2,'0')}`;
  if (remainingSeconds <= 30) el.classList.add('warning');
  else el.classList.remove('warning');
}

// ── Init ──
connectWS();
</script>
</body>
</html>
)rawliteral";

#endif // HTML_DASHBOARD_H
