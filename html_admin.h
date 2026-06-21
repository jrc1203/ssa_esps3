// =============================================================
//  html_admin.h — Admin Portal (served at /admin)
//  Configure game settings, quiz questions, and video URLs
//  without re-uploading code.
// =============================================================
#ifndef HTML_ADMIN_H
#define HTML_ADMIN_H

const char PAGE_ADMIN[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Admin Portal — House of Ancient Secrets</title>
<link href="https://fonts.googleapis.com/css2?family=Cinzel:wght@400;700&family=Inter:wght@300;400;600;700&display=swap" rel="stylesheet">
<style>
*{margin:0;padding:0;box-sizing:border-box;}
:root{
  --bg:#0a0f14;--bg2:#111820;--bg3:#1a2230;
  --gold:#d4af37;--gold-dim:#a6882a;
  --burgundy:#800020;
  --green:#10b981;
  --white:#ffffff;--muted:#a8a29e;--muted2:#6b6560;
  --card-bg:rgba(26,34,48,0.85);
  --border:rgba(212,175,55,0.2);
  --input-bg:rgba(255,255,255,0.06);
}
body{
  font-family:'Inter',sans-serif;background:var(--bg);color:var(--white);
  min-height:100vh;padding-bottom:40px;
}
.header{
  padding:24px 30px;border-bottom:1px solid var(--border);
  background:rgba(10,15,20,0.95);
  display:flex;justify-content:space-between;align-items:center;
}
.header h1{font-family:'Cinzel',serif;font-size:18px;color:var(--gold);letter-spacing:1px;}
.header a{color:var(--muted);font-size:13px;text-decoration:none;}
.header a:hover{color:var(--gold);}

.container{max-width:800px;margin:0 auto;padding:24px 20px;}

/* ── Section Card ── */
.section{
  background:var(--card-bg);border:1px solid var(--border);
  border-radius:14px;padding:24px;margin-bottom:24px;
}
.section-title{
  font-family:'Cinzel',serif;font-size:15px;color:var(--gold);
  margin-bottom:16px;display:flex;align-items:center;gap:10px;
  letter-spacing:1px;
}
.section-title .icon{font-size:20px;}

/* ── Form Elements ── */
.form-row{display:flex;align-items:center;gap:12px;margin-bottom:14px;flex-wrap:wrap;}
.form-label{
  flex:0 0 160px;font-size:13px;color:var(--muted);
}
.form-input{
  flex:1;min-width:100px;padding:10px 14px;
  background:var(--input-bg);border:1px solid rgba(255,255,255,0.1);
  border-radius:8px;color:var(--white);font-size:14px;
  font-family:'Inter',sans-serif;transition:border-color .3s;
}
.form-input:focus{outline:none;border-color:var(--gold-dim);}
.form-input::placeholder{color:var(--muted2);}
.form-hint{font-size:11px;color:var(--muted2);margin-top:2px;margin-left:172px;}

/* ── Question Cards ── */
.q-card{
  background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.06);
  border-radius:10px;padding:16px;margin-bottom:12px;
  position:relative;
}
.q-card .q-num{
  font-size:11px;color:var(--gold-dim);letter-spacing:1px;
  text-transform:uppercase;margin-bottom:8px;
}
.q-card .q-text{font-size:14px;color:var(--white);margin-bottom:10px;line-height:1.4;}
.q-card .q-opts{font-size:12px;color:var(--muted);line-height:1.6;}
.q-card .q-opts .correct{color:var(--green);font-weight:600;}
.q-card .q-actions{
  position:absolute;top:14px;right:14px;display:flex;gap:6px;
}
.q-card .q-actions button{
  padding:4px 10px;border:none;border-radius:6px;font-size:11px;
  cursor:pointer;transition:all .2s;font-family:'Inter',sans-serif;
}
.btn-edit{background:rgba(212,175,55,0.15);color:var(--gold);}
.btn-edit:hover{background:rgba(212,175,55,0.25);}
.btn-del{background:rgba(239,68,68,0.1);color:#ef4444;}
.btn-del:hover{background:rgba(239,68,68,0.2);}

/* ── Add Question Form ── */
.add-form{
  display:none;background:rgba(255,255,255,0.03);
  border:1px dashed rgba(212,175,55,0.3);
  border-radius:10px;padding:16px;margin-top:12px;
}
.add-form.visible{display:block;animation:fadeIn .3s;}
.add-form .form-row{margin-bottom:10px;}
.add-form .form-label{flex:0 0 100px;}

/* ── Buttons ── */
.btn{
  padding:10px 22px;border:none;border-radius:8px;
  font-family:'Inter',sans-serif;font-size:13px;font-weight:600;
  cursor:pointer;transition:all .3s;
}
.btn:active{transform:scale(0.97);}
.btn-primary{background:var(--gold);color:#000;}
.btn-primary:hover{background:var(--gold-dim);}
.btn-secondary{background:var(--bg3);color:var(--muted);border:1px solid var(--border);}
.btn-secondary:hover{color:var(--white);border-color:var(--gold-dim);}
.btn-save{
  display:block;width:100%;padding:14px;font-size:15px;
  background:linear-gradient(135deg,var(--gold),var(--gold-dim));
  color:#000;font-weight:700;letter-spacing:1px;border-radius:10px;
  border:none;cursor:pointer;margin-top:8px;
}
.btn-save:hover{box-shadow:0 0 20px rgba(212,175,55,0.3);}

/* ── Toast ── */
.toast{
  position:fixed;bottom:24px;right:24px;padding:14px 24px;
  border-radius:10px;font-size:14px;font-weight:600;
  z-index:1000;opacity:0;transform:translateY(20px);
  transition:all .4s;
}
.toast.show{opacity:1;transform:translateY(0);}
.toast.success{background:var(--green);color:#fff;}
.toast.error{background:#ef4444;color:#fff;}

/* ── Select ── */
select.form-input{
  appearance:none;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' fill='%23a8a29e' viewBox='0 0 16 16'%3E%3Cpath d='M8 11L3 6h10z'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 12px center;
  padding-right:32px;
}

@keyframes fadeIn{from{opacity:0;}to{opacity:1;}}

@media(max-width:600px){
  .form-row{flex-direction:column;align-items:stretch;gap:4px;}
  .form-label{flex:none;}
  .form-hint{margin-left:0;}
  .add-form .form-label{flex:none;}
}
</style>
</head>
<body>

<div class="header">
  <h1>⚙️ ADMIN PORTAL</h1>
  <a href="/">← Back to Dashboard</a>
</div>

<div class="container">

  <!-- ── Game Settings ── -->
  <div class="section">
    <div class="section-title"><span class="icon">🎮</span> Game Settings</div>
    <div class="form-row">
      <label class="form-label">Game Timer (seconds)</label>
      <input type="number" class="form-input" id="gameTimer" min="30" max="3600" value="300">
    </div>
    <div class="form-row">
      <label class="form-label">Quiz Time (seconds)</label>
      <input type="number" class="form-input" id="quizTime" min="10" max="300" value="30">
    </div>
    <div class="form-row">
      <label class="form-label">Points per Room Hit</label>
      <input type="number" class="form-input" id="ptsRoom" min="0" max="10000" value="100">
    </div>
    <div class="form-row">
      <label class="form-label">Points for Quiz Correct</label>
      <input type="number" class="form-input" id="ptsQuiz" min="0" max="10000" value="200">
    </div>
    <div class="form-row">
      <label class="form-label">Time Bonus Multiplier</label>
      <input type="number" class="form-input" id="ptsTimeMult" min="0" max="100" value="2">
    </div>
    <div class="form-hint">Time Bonus = remaining seconds × multiplier</div>
  </div>

  <!-- ── Hologram Videos ── -->
  <div class="section">
    <div class="section-title"><span class="icon">🎥</span> Hologram Videos</div>
    <p style="font-size:12px;color:var(--muted);margin-bottom:14px;">
      Paste the full YouTube URL for each room's hologram video. 
      Example: https://www.youtube.com/watch?v=dQw4w9WgXcQ
    </p>
    <div class="form-row">
      <label class="form-label">Room 1 — Dakshineswar</label>
      <input type="text" class="form-input" id="video0" placeholder="https://youtube.com/watch?v=...">
    </div>
    <div class="form-row">
      <label class="form-label">Room 2 — Vivekananda</label>
      <input type="text" class="form-input" id="video1" placeholder="https://youtube.com/watch?v=...">
    </div>
    <div class="form-row">
      <label class="form-label">Room 3 — Ramakrishna</label>
      <input type="text" class="form-input" id="video2" placeholder="https://youtube.com/watch?v=...">
    </div>
    <div class="form-row">
      <label class="form-label">Room 4 — Sharada Devi</label>
      <input type="text" class="form-input" id="video3" placeholder="https://youtube.com/watch?v=...">
    </div>
    <div class="form-row">
      <label class="form-label">Room 5 — Belur Math</label>
      <input type="text" class="form-input" id="video4" placeholder="https://youtube.com/watch?v=...">
    </div>
  </div>

  <!-- ── Quiz Questions ── -->
  <div class="section">
    <div class="section-title"><span class="icon">📝</span> Quiz Questions</div>
    <p style="font-size:12px;color:var(--muted);margin-bottom:14px;">
      One question is randomly selected per game. Add as many as you like.
    </p>
    <div id="questionList"></div>

    <button class="btn btn-secondary" onclick="toggleAddForm()" id="btnAddToggle" style="margin-top:12px;">+ Add Question</button>

    <div class="add-form" id="addForm">
      <div class="form-row">
        <label class="form-label">Question</label>
        <input type="text" class="form-input" id="newQ" placeholder="Enter question...">
      </div>
      <div class="form-row">
        <label class="form-label">Option A</label>
        <input type="text" class="form-input" id="newA" placeholder="Option A">
      </div>
      <div class="form-row">
        <label class="form-label">Option B</label>
        <input type="text" class="form-input" id="newB" placeholder="Option B">
      </div>
      <div class="form-row">
        <label class="form-label">Option C</label>
        <input type="text" class="form-input" id="newC" placeholder="Option C">
      </div>
      <div class="form-row">
        <label class="form-label">Correct Answer</label>
        <select class="form-input" id="newAns">
          <option value="0">A</option>
          <option value="1">B</option>
          <option value="2">C</option>
        </select>
      </div>
      <div style="display:flex;gap:8px;margin-top:10px;">
        <button class="btn btn-primary" onclick="addQuestion()">Add</button>
        <button class="btn btn-secondary" onclick="toggleAddForm()">Cancel</button>
      </div>
    </div>
  </div>

  <!-- ── Save Button ── -->
  <button class="btn-save" onclick="saveAll()">💾 SAVE ALL SETTINGS</button>

</div>

<div class="toast" id="toast"></div>

<script>
let questions = [];
let editIndex = -1;

// ── Load config from ESP ──
async function loadConfig() {
  try {
    const res = await fetch('/api/config');
    const data = await res.json();

    document.getElementById('gameTimer').value = data.gameTimer || 300;
    document.getElementById('quizTime').value = data.quizTime || 30;
    document.getElementById('ptsRoom').value = data.ptsRoom || 100;
    document.getElementById('ptsQuiz').value = data.ptsQuiz || 200;
    document.getElementById('ptsTimeMult').value = data.ptsTimeMult || 2;

    if (data.videos) {
      for (let i = 0; i < 5; i++) {
        document.getElementById('video' + i).value = data.videos[i] || '';
      }
    }

    if (data.questions) {
      questions = data.questions;
      renderQuestions();
    }
  } catch(e) {
    showToast('Failed to load config', 'error');
  }
}

// ── Render question cards ──
function renderQuestions() {
  const list = document.getElementById('questionList');
  list.innerHTML = '';
  const labels = ['A','B','C'];

  questions.forEach((q, idx) => {
    const correct = q.ans;
    const card = document.createElement('div');
    card.className = 'q-card';
    card.innerHTML = `
      <div class="q-num">Question ${idx + 1}</div>
      <div class="q-text">${escHtml(q.q)}</div>
      <div class="q-opts">
        <div class="${correct===0?'correct':''}">A) ${escHtml(q.a)}</div>
        <div class="${correct===1?'correct':''}">B) ${escHtml(q.b)}</div>
        <div class="${correct===2?'correct':''}">C) ${escHtml(q.c)}</div>
      </div>
      <div class="q-actions">
        <button class="btn-edit" onclick="editQuestion(${idx})">Edit</button>
        <button class="btn-del" onclick="deleteQuestion(${idx})">Delete</button>
      </div>
    `;
    list.appendChild(card);
  });
}

function escHtml(s) {
  const d = document.createElement('div');
  d.textContent = s || '';
  return d.innerHTML;
}

// ── Add/Edit/Delete questions ──
function toggleAddForm() {
  const form = document.getElementById('addForm');
  form.classList.toggle('visible');
  if (!form.classList.contains('visible')) {
    editIndex = -1;
    clearAddForm();
  }
}

function clearAddForm() {
  document.getElementById('newQ').value = '';
  document.getElementById('newA').value = '';
  document.getElementById('newB').value = '';
  document.getElementById('newC').value = '';
  document.getElementById('newAns').value = '0';
}

function addQuestion() {
  const q = document.getElementById('newQ').value.trim();
  const a = document.getElementById('newA').value.trim();
  const b = document.getElementById('newB').value.trim();
  const c = document.getElementById('newC').value.trim();
  const ans = parseInt(document.getElementById('newAns').value);

  if (!q || !a || !b || !c) {
    showToast('Please fill all fields', 'error');
    return;
  }

  const obj = {q, a, b, c, ans};

  if (editIndex >= 0) {
    questions[editIndex] = obj;
    editIndex = -1;
  } else {
    questions.push(obj);
  }

  renderQuestions();
  clearAddForm();
  document.getElementById('addForm').classList.remove('visible');
}

function editQuestion(idx) {
  editIndex = idx;
  const q = questions[idx];
  document.getElementById('newQ').value = q.q;
  document.getElementById('newA').value = q.a;
  document.getElementById('newB').value = q.b;
  document.getElementById('newC').value = q.c;
  document.getElementById('newAns').value = q.ans;
  document.getElementById('addForm').classList.add('visible');
  document.getElementById('addForm').scrollIntoView({behavior:'smooth'});
}

function deleteQuestion(idx) {
  if (confirm('Delete question ' + (idx + 1) + '?')) {
    questions.splice(idx, 1);
    renderQuestions();
  }
}

// ── Save all settings ──
async function saveAll() {
  const payload = {
    gameTimer: parseInt(document.getElementById('gameTimer').value),
    quizTime: parseInt(document.getElementById('quizTime').value),
    ptsRoom: parseInt(document.getElementById('ptsRoom').value),
    ptsQuiz: parseInt(document.getElementById('ptsQuiz').value),
    ptsTimeMult: parseInt(document.getElementById('ptsTimeMult').value),
    videos: [
      document.getElementById('video0').value,
      document.getElementById('video1').value,
      document.getElementById('video2').value,
      document.getElementById('video3').value,
      document.getElementById('video4').value
    ],
    questions: questions
  };

  try {
    const res = await fetch('/api/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(payload)
    });
    if (res.ok) {
      showToast('Settings saved successfully!', 'success');
    } else {
      showToast('Save failed: ' + res.status, 'error');
    }
  } catch(e) {
    showToast('Network error: ' + e.message, 'error');
  }
}

// ── Toast notification ──
function showToast(msg, type) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast ' + type + ' show';
  setTimeout(() => { t.classList.remove('show'); }, 3000);
}

// ── Init ──
loadConfig();
</script>
</body>
</html>
)rawliteral";

#endif // HTML_ADMIN_H
