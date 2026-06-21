// =============================================================
//  html_hologram.h — Hologram Video Player (served at /hologram)
//  Pure black screen, auto-plays YouTube videos via WebSocket
//  This page runs on the mobile phone inside the hologram box.
// =============================================================
#ifndef HTML_HOLOGRAM_H
#define HTML_HOLOGRAM_H

const char PAGE_HOLOGRAM[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black">
<title>Hologram Player</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
html,body{
  width:100%;height:100%;background:#000;overflow:hidden;
  -webkit-user-select:none;user-select:none;
}
/* Connection indicator — tiny dot in corner */
.conn-indicator{
  position:fixed;top:8px;left:8px;z-index:999;
  width:6px;height:6px;border-radius:50%;
  background:#333;opacity:0.5;transition:all .5s;
}
.conn-indicator.online{background:#10b981;opacity:1;}

/* ── Fullscreen splash — shown once on load ── */
#fsSplash{
  position:fixed;top:0;left:0;width:100%;height:100%;
  z-index:9999;background:#000;
  display:flex;flex-direction:column;align-items:center;justify-content:center;
  cursor:pointer;-webkit-tap-highlight-color:transparent;
}
#fsSplash .ring{
  width:80px;height:80px;border-radius:50%;
  border:2px solid rgba(212,175,55,0.3);
  display:flex;align-items:center;justify-content:center;
  animation:pulseRing 2s ease-in-out infinite;
}
#fsSplash .ring svg{
  width:36px;height:36px;fill:rgba(212,175,55,0.7);
}
#fsSplash .label{
  margin-top:20px;font-family:system-ui,sans-serif;
  font-size:14px;color:rgba(212,175,55,0.6);
  letter-spacing:2px;text-transform:uppercase;
}
@keyframes pulseRing{
  0%,100%{border-color:rgba(212,175,55,0.15);transform:scale(1);}
  50%{border-color:rgba(212,175,55,0.5);transform:scale(1.08);}
}

/* Video container — fills entire screen */
#videoContainer{
  position:fixed;top:0;left:0;width:100%;height:100%;
  display:none;z-index:10;
}
#videoContainer iframe{
  width:100%;height:100%;border:none;
}

/* Waiting screen — subtle breathing animation */
.waiting{
  position:fixed;top:0;left:0;width:100%;height:100%;
  display:flex;align-items:center;justify-content:center;
  z-index:5;
}
.waiting-dot{
  width:4px;height:4px;border-radius:50%;
  background:rgba(212,175,55,0.3);
  animation:breathe 3s ease-in-out infinite;
}
@keyframes breathe{
  0%,100%{opacity:0.1;transform:scale(1);}
  50%{opacity:0.4;transform:scale(1.5);}
}

/* Status overlay for debugging */
#statusOverlay{
  position:fixed;bottom:8px;right:8px;z-index:999;
  font-family:monospace;font-size:10px;color:#333;
  background:rgba(0,0,0,0.8);padding:4px 8px;border-radius:4px;
  display:none;
}
</style>
</head>
<body>

<!-- Fullscreen splash — one tap to lock into fullscreen -->
<div id="fsSplash">
  <div class="ring">
    <svg viewBox="0 0 24 24"><path d="M7 14H5v5h5v-2H7v-3zm-2-4h2V7h3V5H5v5zm12 7h-3v2h5v-5h-2v3zM14 5v2h3v3h2V5h-5z"/></svg>
  </div>
  <div class="label">Tap to Start</div>
</div>

<div class="conn-indicator" id="connDot"></div>
<div class="waiting" id="waitingScreen"><div class="waiting-dot"></div></div>

<div id="videoContainer"></div>
<div id="statusOverlay"></div>

<script>
// ── State ──
let videoIds = ['','','','',''];
let currentPlayer = null;
let isPlaying = false;
let fullscreenLocked = false;
let wakeLock = null;
let ws;
let reconnectTimer;

// ── Fullscreen + Wake Lock ──
// Request fullscreen on the document element
function enterFullscreen() {
  const el = document.documentElement;
  const rfs = el.requestFullscreen || el.webkitRequestFullscreen || el.msRequestFullscreen;
  if (rfs) {
    return rfs.call(el).catch(()=>{});
  }
  return Promise.resolve();
}

// Request Wake Lock so the phone screen never dims/sleeps
async function acquireWakeLock() {
  if ('wakeLock' in navigator) {
    try {
      wakeLock = await navigator.wakeLock.request('screen');
      wakeLock.addEventListener('release', () => { log('Wake lock released'); });
      log('Wake lock acquired');
    } catch(e) {
      log('Wake lock failed: ' + e.message);
    }
  }
}

// Re-acquire wake lock when page becomes visible again
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'visible' && fullscreenLocked) {
    acquireWakeLock();
  }
});

// Re-enter fullscreen if user accidentally exits (e.g. swipe gesture)
document.addEventListener('fullscreenchange', () => {
  if (!document.fullscreenElement && fullscreenLocked) {
    // Show splash again so user can re-enter with a tap
    document.getElementById('fsSplash').style.display = 'flex';
  }
});

// ── Splash tap handler ──
document.getElementById('fsSplash').addEventListener('click', async () => {
  await enterFullscreen();
  await acquireWakeLock();
  // Lock screen orientation to landscape if API available
  if (screen.orientation && screen.orientation.lock) {
    screen.orientation.lock('landscape').catch(()=>{});
  }
  fullscreenLocked = true;
  document.getElementById('fsSplash').style.display = 'none';
  log('Fullscreen locked');
});

// ── YouTube IFrame API ──
const tag = document.createElement('script');
tag.src = 'https://www.youtube.com/iframe_api';
document.head.appendChild(tag);

let ytReady = false;
function onYouTubeIframeAPIReady() {
  ytReady = true;
  log('YouTube API ready');
}

// ── Play a video by room index (0–4) ──
function playRoomVideo(roomIndex) {
  const videoId = videoIds[roomIndex];
  if (!videoId || videoId.length < 5) {
    log('No video ID for room ' + (roomIndex + 1));
    return;
  }

  // Re-ensure fullscreen is active before playing
  if (!document.fullscreenElement && fullscreenLocked) {
    enterFullscreen().catch(()=>{});
  }

  isPlaying = true;
  const container = document.getElementById('videoContainer');
  // Hide the waiting dot while video plays
  document.getElementById('waitingScreen').style.display = 'none';
  container.style.display = 'block';
  container.innerHTML = '';

  const playerDiv = document.createElement('div');
  playerDiv.id = 'ytPlayer';
  playerDiv.style.width = '100%';
  playerDiv.style.height = '100%';
  container.appendChild(playerDiv);

  if (ytReady) {
    currentPlayer = new YT.Player('ytPlayer', {
      videoId: videoId,
      playerVars: {
        autoplay: 1,
        controls: 0,
        modestbranding: 1,
        rel: 0,
        showinfo: 0,
        fs: 0,
        iv_load_policy: 3,
        playsinline: 1
      },
      events: {
        onReady: (e) => {
          e.target.playVideo();
          log('Playing room ' + (roomIndex + 1));
        },
        onStateChange: (e) => {
          if (e.data === 0) { stopVideo(); }
        },
        onError: (e) => {
          log('YT error: ' + e.data);
          stopVideo();
        }
      }
    });
  } else {
    container.innerHTML = `<iframe src="https://www.youtube.com/embed/${videoId}?autoplay=1&controls=0&modestbranding=1&rel=0&playsinline=1" allow="autoplay;encrypted-media" allowfullscreen></iframe>`;
    log('Playing room ' + (roomIndex + 1) + ' (iframe fallback)');
    setTimeout(stopVideo, 60000);
  }
}

function stopVideo() {
  isPlaying = false;
  const container = document.getElementById('videoContainer');
  if (currentPlayer && currentPlayer.destroy) {
    try { currentPlayer.destroy(); } catch(e) {}
    currentPlayer = null;
  }
  container.innerHTML = '';
  container.style.display = 'none';
  // Restore waiting dot
  document.getElementById('waitingScreen').style.display = 'flex';
  log('Video stopped — black screen');
}

// ── Extract YouTube ID from full URL ──
function extractId(url) {
  if (!url) return '';
  let m = url.match(/[?&]v=([^&]+)/);
  if (m) return m[1];
  m = url.match(/youtu\.be\/([^?]+)/);
  if (m) return m[1];
  m = url.match(/\/embed\/([^?]+)/);
  if (m) return m[1];
  if (url.length >= 11 && !url.includes('/')) return url.trim();
  return url.trim();
}

// ── WebSocket Connection ──
function connectWS() {
  const host = window.location.hostname;
  const port = window.location.port || '80';
  ws = new WebSocket(`ws://${host}:${port}/ws`);

  ws.onopen = () => {
    document.getElementById('connDot').classList.add('online');
    log('WS connected');
    ws.send(JSON.stringify({action:'get_videos'}));
  };

  ws.onclose = () => {
    document.getElementById('connDot').classList.remove('online');
    clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(connectWS, 3000);
  };

  ws.onerror = () => { ws.close(); };

  ws.onmessage = (e) => {
    try {
      const data = JSON.parse(e.data);
      handleEvent(data);
    } catch(err) {
      log('Parse error: ' + err);
    }
  };
}

function handleEvent(data) {
  switch(data.event) {
    case 'room_hit':
      if (!isPlaying && data.room !== undefined) {
        playRoomVideo(data.room);
      }
      break;

    case 'videos':
      if (data.urls && Array.isArray(data.urls)) {
        for (let i = 0; i < 5 && i < data.urls.length; i++) {
          videoIds[i] = extractId(data.urls[i]);
        }
        log('Video IDs loaded: ' + videoIds.filter(v=>v).length + '/5');
      }
      break;

    case 'game_reset':
      stopVideo();
      break;

    case 'game_over':
      setTimeout(stopVideo, 2000);
      break;
  }
}

function log(msg) {
  console.log('[Hologram] ' + msg);
  const el = document.getElementById('statusOverlay');
  el.textContent = msg;
  el.style.display = 'block';
  setTimeout(() => { el.style.display = 'none'; }, 3000);
}

// ── Double-tap to toggle fullscreen (backup) ──
let lastTap = 0;
document.body.addEventListener('touchend', (e) => {
  if (e.target.closest('#fsSplash')) return;
  const now = Date.now();
  if (now - lastTap < 300) {
    if (document.fullscreenElement) {
      document.exitFullscreen();
      fullscreenLocked = false;
    } else {
      enterFullscreen();
      fullscreenLocked = true;
    }
  }
  lastTap = now;
});

// ── Init ──
connectWS();
</script>
</body>
</html>
)rawliteral";

#endif // HTML_HOLOGRAM_H
