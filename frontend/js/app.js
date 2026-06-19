const API = '';  // same origin — C server serves both frontend and API

/* ============================================================
   UTILS
============================================================ */
async function api(method, path, body) {
  try {
    const opts = { method, headers: { 'Content-Type': 'application/json' } };
    if (body) opts.body = JSON.stringify(body);
    const res = await fetch(API + path, opts);
    return await res.json();
  } catch(e) {
    setServerStatus(false);
    return { error: e.message };
  }
}

function toast(msg, type = 'success') {
  const t = document.createElement('div');
  t.className = `toast ${type}`;
  t.textContent = msg;
  document.body.appendChild(t);
  setTimeout(() => t.remove(), 3000);
}

function badge(val, type) {
  return `<span class="badge badge-${type.toLowerCase()}">${val}</span>`;
}

function priorityBadge(p) {
  const map = { EMERGENCY: 'emergency', VIP: 'vip', NORMAL: 'normal' };
  return badge(p, map[p] || 'normal');
}

function statusBadge(s) {
  const map = { AVAILABLE:'available', BUSY:'busy', BREAK:'break', OFFLINE:'offline',
                WAITING:'waiting', ACTIVE:'active', COMPLETED:'completed', ABANDONED:'abandoned' };
  return badge(s, map[s] || 'normal');
}

function skillTags(skills) {
  if (!skills) return '—';
  return skills.split(',').filter(Boolean).map(s => `<span class="skill-tag">${s.trim()}</span>`).join('');
}

function fmtTime(sec) {
  if (!sec) return '—';
  if (sec < 60) return `${sec}s`;
  return `${Math.floor(sec/60)}m ${sec%60}s`;
}

function fmtDate(d) {
  if (!d) return '—';
  return new Date(d).toLocaleString();
}

function setServerStatus(ok) {
  const dot  = document.querySelector('.status-dot');
  const txt  = document.getElementById('server-status-text');
  if (dot) dot.className = 'status-dot ' + (ok ? 'online' : '');
  if (txt) txt.textContent = ok ? 'Connected' : 'Disconnected';
}

/* ============================================================
   NAVIGATION
============================================================ */
let currentPage = 'dashboard';

document.querySelectorAll('.nav-item').forEach(item => {
  item.addEventListener('click', () => {
    const page = item.dataset.page;
    navigateTo(page);
  });
});

function navigateTo(page) {
  document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
  document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
  document.querySelector(`[data-page="${page}"]`).classList.add('active');
  document.getElementById(`page-${page}`).classList.add('active');
  document.getElementById('page-title').textContent = page.charAt(0).toUpperCase() + page.slice(1);
  currentPage = page;
  loadPage(page);
}

function loadPage(page) {
  const loaders = {
    dashboard: loadDashboard,
    agents:    loadAgents,
    calls:     loadCalls,
    queue:     loadQueue,
    dispatch:  loadDispatch,
    reports:   loadReports,
  };
  if (loaders[page]) loaders[page]();
}

/* ============================================================
   CLOCK
============================================================ */
function updateClock() {
  const el = document.getElementById('clock');
  if (el) el.textContent = new Date().toLocaleTimeString();
}
setInterval(updateClock, 1000);
updateClock();

/* ============================================================
   DASHBOARD
============================================================ */
async function loadDashboard() {
  const data = await api('GET', '/api/reports/dashboard');
  if (data.error) { setServerStatus(false); return; }
  setServerStatus(true);

  document.getElementById('stat-total').textContent     = data.total_calls     ?? 0;
  document.getElementById('stat-waiting').textContent   = data.waiting         ?? 0;
  document.getElementById('stat-active').textContent    = data.active          ?? 0;
  document.getElementById('stat-completed').textContent = data.completed       ?? 0;
  document.getElementById('stat-abandoned').textContent = data.abandoned       ?? 0;

  document.getElementById('agent-status-grid').innerHTML = `
    <div class="agent-status-item">
      <div class="agent-status-num color-avail">${data.agents_available ?? 0}</div>
      <div class="agent-status-lbl">Available</div>
    </div>
    <div class="agent-status-item">
      <div class="agent-status-num color-busy">${data.agents_busy ?? 0}</div>
      <div class="agent-status-lbl">Busy</div>
    </div>
    <div class="agent-status-item">
      <div class="agent-status-num color-break">${data.agents_break ?? 0}</div>
      <div class="agent-status-lbl">On Break</div>
    </div>
    <div class="agent-status-item" style="grid-column:1/-1">
      <div class="agent-status-num color-offline">${data.agents_offline ?? 0}</div>
      <div class="agent-status-lbl">Offline</div>
    </div>
  `;

  document.getElementById('metrics-list').innerHTML = `
    <div class="metric-row">
      <span class="metric-label">Avg Wait Time</span>
      <span class="metric-value">${fmtTime(Math.round(data.avg_wait_time))}</span>
    </div>
    <div class="metric-row">
      <span class="metric-label">Avg Handle Time</span>
      <span class="metric-value">${fmtTime(Math.round(data.avg_handle_time))}</span>
    </div>
    <div class="metric-row">
      <span class="metric-label">Resolution Rate</span>
      <span class="metric-value">${data.total_calls ? Math.round(data.completed/data.total_calls*100) : 0}%</span>
    </div>
    <div class="metric-row">
      <span class="metric-label">Abandon Rate</span>
      <span class="metric-value">${data.total_calls ? Math.round(data.abandoned/data.total_calls*100) : 0}%</span>
    </div>
  `;

  const queue = await api('GET', '/api/queue');
  const el = document.getElementById('dash-queue-table');
  if (!queue.length) {
    el.innerHTML = '<div class="empty-state"><div class="empty-icon">✅</div>No calls waiting</div>';
    return;
  }
  el.innerHTML = `<div class="tbl-wrap"><table>
    <thead><tr><th>#</th><th>Caller</th><th>Name</th><th>Priority</th><th>Category</th><th>Queued At</th><th>Action</th></tr></thead>
    <tbody>${queue.map(c=>`<tr>
      <td>${c.id}</td>
      <td>${c.caller_id}</td>
      <td>${c.caller_name}</td>
      <td>${priorityBadge(c.priority)}</td>
      <td>${c.category}</td>
      <td>${fmtDate(c.created_at)}</td>
      <td><button class="btn btn-xs btn-primary" onclick="quickDispatch(${c.id},'${c.category}')">Dispatch</button></td>
    </tr>`).join('')}</tbody>
  </table></div>`;
}

/* ============================================================
   AGENTS
============================================================ */
async function loadAgents() {
  const agents = await api('GET', '/api/agents');
  const el = document.getElementById('agents-table');
  if (!agents.length) {
    el.innerHTML = '<div class="empty-state"><div class="empty-icon">👤</div>No agents yet. Add one!</div>';
    return;
  }
  el.innerHTML = `<div class="tbl-wrap"><table>
    <thead><tr><th>ID</th><th>Name</th><th>Phone</th><th>Email</th><th>Status</th><th>Skills</th><th>Calls</th><th>Actions</th></tr></thead>
    <tbody>${agents.map(a=>`<tr>
      <td>${a.id}</td>
      <td><strong>${a.name}</strong></td>
      <td>${a.phone||'—'}</td>
      <td>${a.email||'—'}</td>
      <td>
        <select class="badge" onchange="changeAgentStatus(${a.id}, this.value)" style="border:none;background:transparent;cursor:pointer;font-size:11px;font-weight:600;padding:2px 4px;">
          ${['AVAILABLE','BUSY','BREAK','OFFLINE'].map(s=>`<option value="${s}" ${a.status===s?'selected':''}>${s}</option>`).join('')}
        </select>
      </td>
      <td>${skillTags(a.skills)}</td>
      <td>${a.calls_handled}</td>
      <td style="display:flex;gap:6px;flex-wrap:wrap;">
        <button class="btn btn-xs btn-outline" onclick="openAgentModal(${a.id})">Edit</button>
        <button class="btn btn-xs btn-danger"  onclick="deleteAgent(${a.id})">Delete</button>
      </td>
    </tr>`).join('')}</tbody>
  </table></div>`;
}

function openAgentModal(id = null) {
  document.getElementById('agent-id').value    = id || '';
  document.getElementById('agent-name').value  = '';
  document.getElementById('agent-phone').value = '';
  document.getElementById('agent-email').value = '';
  document.getElementById('agent-skills').value= '';
  document.getElementById('agent-modal-title').textContent = id ? 'Edit Agent' : 'Add Agent';

  if (id) {
    api('GET', `/api/agents/${id}`).then(a => {
      document.getElementById('agent-name').value  = a.name  || '';
      document.getElementById('agent-phone').value = a.phone || '';
      document.getElementById('agent-email').value = a.email || '';
      document.getElementById('agent-skills').value= a.skills|| '';
    });
  }
  openModal('agent-modal');
}

async function saveAgent() {
  const id     = document.getElementById('agent-id').value;
  const body   = {
    name:   document.getElementById('agent-name').value.trim(),
    phone:  document.getElementById('agent-phone').value.trim(),
    email:  document.getElementById('agent-email').value.trim(),
    skills: document.getElementById('agent-skills').value.trim(),
  };
  if (!body.name) { toast('Name is required', 'error'); return; }
  const res = id ? await api('PUT', `/api/agents/${id}`, body)
                 : await api('POST', '/api/agents', body);
  if (res.success || res.id) {
    toast(id ? 'Agent updated' : 'Agent added');
    closeModal();
    loadAgents();
  } else {
    toast(res.error || 'Failed', 'error');
  }
}

async function deleteAgent(id) {
  if (!confirm('Delete this agent?')) return;
  const res = await api('DELETE', `/api/agents/${id}`);
  if (res.success) { toast('Agent deleted'); loadAgents(); }
  else toast(res.error || 'Failed', 'error');
}

async function changeAgentStatus(id, status) {
  const res = await api('PUT', `/api/agents/status/${id}`, { status });
  if (res.success) toast(`Status → ${status}`);
  else toast(res.error || 'Failed', 'error');
}

/* ============================================================
   CALLS
============================================================ */
let allCalls = [];

async function loadCalls() {
  allCalls = await api('GET', '/api/calls');
  renderCallsTable(allCalls);
}

function renderCallsTable(calls) {
  const el = document.getElementById('calls-table');
  if (!calls.length) {
    el.innerHTML = '<div class="empty-state"><div class="empty-icon">📋</div>No calls found</div>';
    return;
  }
  el.innerHTML = `<div class="tbl-wrap"><table>
    <thead><tr><th>ID</th><th>Caller</th><th>Name</th><th>Priority</th><th>Category</th><th>Status</th><th>Agent</th><th>Wait</th><th>Handle</th><th>Created</th><th>Actions</th></tr></thead>
    <tbody>${calls.map(c=>`<tr>
      <td>${c.id}</td>
      <td>${c.caller_id}</td>
      <td>${c.caller_name||'—'}</td>
      <td>${priorityBadge(c.priority)}</td>
      <td>${c.category}</td>
      <td>${statusBadge(c.status)}</td>
      <td>${c.agent_name||'—'}</td>
      <td>${fmtTime(c.wait_time)}</td>
      <td>${fmtTime(c.handle_time)}</td>
      <td>${fmtDate(c.created_at)}</td>
      <td style="display:flex;gap:4px;flex-wrap:wrap;">
        ${c.status==='ACTIVE'  ? `<button class="btn btn-xs btn-success" onclick="resolveCall(${c.id})">Resolve</button>` : ''}
        ${c.status==='WAITING' ? `<button class="btn btn-xs btn-primary" onclick="quickDispatch(${c.id},'${c.category}')">Dispatch</button>` : ''}
        ${c.status==='WAITING'||c.status==='ACTIVE' ? `<button class="btn btn-xs btn-danger" onclick="abandonCall(${c.id})">Abandon</button>` : ''}
      </td>
    </tr>`).join('')}</tbody>
  </table></div>`;
}

function filterCalls() {
  const search   = document.getElementById('call-search').value.toLowerCase();
  const status   = document.getElementById('call-filter-status').value;
  const priority = document.getElementById('call-filter-priority').value;
  const filtered = allCalls.filter(c =>
    (!search   || c.caller_id.toLowerCase().includes(search) || (c.caller_name||'').toLowerCase().includes(search)) &&
    (!status   || c.status   === status) &&
    (!priority || c.priority === priority)
  );
  renderCallsTable(filtered);
}

function openCallModal() {
  document.getElementById('call-caller-id').value   = '';
  document.getElementById('call-caller-name').value = '';
  document.getElementById('call-priority').value    = 'NORMAL';
  document.getElementById('call-category').value    = 'general';
  document.getElementById('call-notes').value       = '';
  openModal('call-modal');
}

async function saveCall() {
  const body = {
    caller_id:   document.getElementById('call-caller-id').value.trim(),
    caller_name: document.getElementById('call-caller-name').value.trim(),
    priority:    document.getElementById('call-priority').value,
    category:    document.getElementById('call-category').value,
    notes:       document.getElementById('call-notes').value.trim(),
  };
  if (!body.caller_id) { toast('Caller ID required', 'error'); return; }
  const res = await api('POST', '/api/calls', body);
  if (res.id || res.success) {
    toast('Call added to queue');
    closeModal();
    loadCalls();
  } else {
    toast(res.error || 'Failed', 'error');
  }
}

async function resolveCall(id) {
  const res = await api('PUT', `/api/calls/resolve/${id}`);
  if (res.success) { toast('Call resolved ✓'); loadCalls(); loadDashboard(); }
  else toast(res.error || 'Failed', 'error');
}

async function abandonCall(id) {
  if (!confirm('Mark call as abandoned?')) return;
  const res = await api('PUT', `/api/calls/abandon/${id}`);
  if (res.success) { toast('Call abandoned'); loadCalls(); loadDashboard(); }
  else toast(res.error || 'Failed', 'error');
}

/* ============================================================
   QUEUE
============================================================ */
async function loadQueue() {
  const queue = await api('GET', '/api/queue');
  const el = document.getElementById('queue-table');
  if (!queue.length) {
    el.innerHTML = '<div class="empty-state"><div class="empty-icon">✅</div>Queue is empty</div>';
    return;
  }
  el.innerHTML = `<div class="tbl-wrap"><table>
    <thead><tr><th>Position</th><th>ID</th><th>Caller</th><th>Name</th><th>Priority</th><th>Category</th><th>Queued At</th><th>Action</th></tr></thead>
    <tbody>${queue.map((c,i)=>`<tr>
      <td><strong>#${i+1}</strong></td>
      <td>${c.id}</td>
      <td>${c.caller_id}</td>
      <td>${c.caller_name}</td>
      <td>${priorityBadge(c.priority)}</td>
      <td>${c.category}</td>
      <td>${fmtDate(c.created_at)}</td>
      <td style="display:flex;gap:4px;">
        <button class="btn btn-xs btn-primary" onclick="quickDispatch(${c.id},'${c.category}')">Dispatch</button>
        <button class="btn btn-xs btn-danger"  onclick="abandonCall(${c.id})">Abandon</button>
      </td>
    </tr>`).join('')}</tbody>
  </table></div>`;
}

/* ============================================================
   DISPATCH
============================================================ */
async function loadDispatch() {
  const [queue, agents] = await Promise.all([
    api('GET', '/api/queue'),
    api('GET', '/api/agents'),
  ]);

  const callSel  = document.getElementById('dispatch-call-id');
  const agentSel = document.getElementById('dispatch-agent-id');

  callSel.innerHTML  = '<option value="">-- select call --</option>' +
    queue.map(c=>`<option value="${c.id}">[${c.priority}] #${c.id} - ${c.caller_id} (${c.category})</option>`).join('');
  agentSel.innerHTML = '<option value="">-- auto select --</option>' +
    agents.filter(a=>a.status==='AVAILABLE').map(a=>`<option value="${a.id}">${a.name} [${a.skills||'general'}]</option>`).join('');

  const ql = document.getElementById('dispatch-queue-list');
  ql.innerHTML = queue.length
    ? queue.map(c=>`<div class="dispatch-list-item">
        <div>${priorityBadge(c.priority)} <strong>${c.caller_id}</strong> — ${c.caller_name}</div>
        <div style="color:var(--text-light);font-size:12px;">${c.category}</div>
      </div>`).join('')
    : '<div class="empty-state">Queue empty</div>';

  const al = document.getElementById('dispatch-agents-list');
  const avail = agents.filter(a=>a.status==='AVAILABLE');
  al.innerHTML = avail.length
    ? avail.map(a=>`<div class="dispatch-list-item">
        <div><strong>${a.name}</strong> ${skillTags(a.skills)}</div>
        ${statusBadge(a.status)}
      </div>`).join('')
    : '<div class="empty-state">No agents available</div>';
}

async function manualDispatch() {
  const call_id  = parseInt(document.getElementById('dispatch-call-id').value);
  const agent_id = parseInt(document.getElementById('dispatch-agent-id').value) || undefined;
  const res = document.getElementById('dispatch-result');
  if (!call_id) { res.className='dispatch-result error'; res.textContent='Select a call first.'; return; }

  const body = { call_id };
  if (agent_id) body.agent_id = agent_id;

  const data = await api('POST', '/api/dispatch', body);
  if (data.success) {
    res.className = 'dispatch-result success';
    res.textContent = `Dispatched call #${data.call_id} → Agent #${data.agent_id}`;
    loadDispatch();
    loadDashboard();
  } else {
    res.className = 'dispatch-result error';
    res.textContent = data.error || 'Dispatch failed';
  }
}

async function quickDispatch(call_id, category) {
  const data = await api('POST', '/api/dispatch', { call_id, category });
  if (data.success) {
    toast(`Call #${call_id} dispatched → Agent #${data.agent_id}`);
    refreshAll();
  } else {
    toast(data.error || 'No available agent', 'error');
  }
}

async function autoDispatch() {
  const data = await api('POST', '/api/dispatch/auto');
  if (data.success) {
    toast(`Auto-dispatched ${data.dispatched} call(s)`);
    refreshAll();
  } else {
    toast(data.error || 'Failed', 'error');
  }
}

/* ============================================================
   REPORTS
============================================================ */
async function loadReports() {
  const [daily, agents] = await Promise.all([
    api('GET', '/api/reports/daily'),
    api('GET', '/api/reports/agents'),
  ]);

  const dt = document.getElementById('daily-report-table');
  dt.innerHTML = daily.length
    ? `<div class="tbl-wrap"><table>
        <thead><tr><th>Date</th><th>Total</th><th>Resolved</th><th>Abandoned</th><th>Avg Wait</th><th>Avg Handle</th></tr></thead>
        <tbody>${daily.map(d=>`<tr>
          <td>${d.date}</td>
          <td>${d.total}</td>
          <td style="color:var(--green)">${d.resolved}</td>
          <td style="color:var(--red)">${d.abandoned}</td>
          <td>${fmtTime(Math.round(d.avg_wait))}</td>
          <td>${fmtTime(Math.round(d.avg_handle))}</td>
        </tr>`).join('')}</tbody>
      </table></div>`
    : '<div class="empty-state">No data yet</div>';

  const at = document.getElementById('agent-report-table');
  at.innerHTML = agents.length
    ? `<div class="tbl-wrap"><table>
        <thead><tr><th>Agent</th><th>Status</th><th>Skills</th><th>Total Calls</th><th>Resolved</th><th>Avg Handle</th></tr></thead>
        <tbody>${agents.map(a=>`<tr>
          <td><strong>${a.name}</strong></td>
          <td>${statusBadge(a.status)}</td>
          <td>${skillTags(a.skills)}</td>
          <td>${a.total_calls}</td>
          <td style="color:var(--green)">${a.resolved}</td>
          <td>${fmtTime(Math.round(a.avg_handle))}</td>
        </tr>`).join('')}</tbody>
      </table></div>`
    : '<div class="empty-state">No data yet</div>';
}

/* ============================================================
   MODAL HELPERS
============================================================ */
function openModal(id) {
  document.getElementById('modal-overlay').classList.add('open');
  document.getElementById(id).classList.add('open');
}

function closeModal() {
  document.getElementById('modal-overlay').classList.remove('open');
  document.querySelectorAll('.modal').forEach(m => m.classList.remove('open'));
}

/* ============================================================
   REFRESH ALL (called from topbar button)
============================================================ */
function refreshAll() {
  loadPage(currentPage);
}

/* ============================================================
   AUTO REFRESH every 15s
============================================================ */
setInterval(() => {
  if (currentPage === 'dashboard') loadDashboard();
  if (currentPage === 'queue')     loadQueue();
}, 15000);

/* ============================================================
   INIT
============================================================ */
loadDashboard();
