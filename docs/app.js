/* MIDI-Mod configurator
 *
 * Builds the config.toml that main/configManager.cpp parses. The MESSAGES
 * table below mirrors the firmware exactly: same tokens, same byte counts,
 * same rules for where each data byte comes from.
 */

'use strict';

const MODULE_COUNT = 3;
const DEVICE_COUNT = 5;
const STORE_KEY = 'midimod.config.v1';

/* ---------------------------------------------------------------- messages */

// src describes where a data byte comes from when nothing overrides it:
//   data     - the number configured on the device
//   velocity - the module's press_velocity
//   input    - the live 0-127 reading
//   lsb/msb  - the reading spread over 14 bits
const MESSAGES = [
  { id: 'noop', label: 'None', group: 'Off', status: null, len: 0, dataKind: 'none' },

  { id: 'note_on', label: 'Note On', group: 'Channel voice', status: 0x90, len: 3,
    dataKind: 'note', d0: { src: 'data', label: 'note' }, d1: { src: 'velocity', label: 'velocity' } },
  { id: 'note_off', label: 'Note Off', group: 'Channel voice', status: 0x80, len: 3,
    dataKind: 'note', d0: { src: 'data', label: 'note' }, d1: { src: 'velocity', label: 'velocity' } },
  { id: 'poly_aftertouch', label: 'Poly Aftertouch', group: 'Channel voice', status: 0xA0, len: 3,
    dataKind: 'note', d0: { src: 'data', label: 'note' }, d1: { src: 'input', label: 'pressure' } },
  { id: 'cc', label: 'Control Change', group: 'Channel voice', status: 0xB0, len: 3,
    dataKind: 'cc', d0: { src: 'data', label: 'controller' }, d1: { src: 'input', label: 'value' } },
  { id: 'pc', label: 'Program Change', group: 'Channel voice', status: 0xC0, len: 2,
    dataKind: 'program', d0: { src: 'data', label: 'program' } },
  { id: 'channel_aftertouch', label: 'Channel Aftertouch', group: 'Channel voice', status: 0xD0, len: 2,
    dataKind: 'none', d0: { src: 'input', label: 'pressure' } },
  { id: 'pitch_bend', label: 'Pitch Bend', group: 'Channel voice', status: 0xE0, len: 3,
    dataKind: 'none', d0: { src: 'lsb', label: 'bend LSB' }, d1: { src: 'msb', label: 'bend MSB' } },

  { id: 'mtc_quarter_frame', label: 'MTC Quarter Frame', group: 'System common', status: 0xF1, len: 2,
    dataKind: 'raw', d0: { src: 'data', label: 'time code' } },
  { id: 'song_position', label: 'Song Position', group: 'System common', status: 0xF2, len: 3,
    dataKind: 'none', d0: { src: 'lsb', label: 'position LSB' }, d1: { src: 'msb', label: 'position MSB' } },
  { id: 'song_select', label: 'Song Select', group: 'System common', status: 0xF3, len: 2,
    dataKind: 'raw', d0: { src: 'data', label: 'song' } },
  { id: 'tune_request', label: 'Tune Request', group: 'System common', status: 0xF6, len: 1, dataKind: 'none' },

  { id: 'clock', label: 'Timing Clock', group: 'System real time', status: 0xF8, len: 1, dataKind: 'none' },
  { id: 'start', label: 'Start', group: 'System real time', status: 0xFA, len: 1, dataKind: 'none' },
  { id: 'continue', label: 'Continue', group: 'System real time', status: 0xFB, len: 1, dataKind: 'none' },
  { id: 'stop', label: 'Stop', group: 'System real time', status: 0xFC, len: 1, dataKind: 'none' },
  { id: 'active_sensing', label: 'Active Sensing', group: 'System real time', status: 0xFE, len: 1, dataKind: 'none' },
  { id: 'system_reset', label: 'System Reset', group: 'System real time', status: 0xFF, len: 1, dataKind: 'none' },
];

// Extra spellings the firmware accepts, for import only.
const ALIASES = {
  control_change: 'cc',
  program_change: 'pc',
  channel_pressure: 'channel_aftertouch',
  timing_clock: 'clock',
};

const MSG_BY_ID = Object.fromEntries(MESSAGES.map((m) => [m.id, m]));

function msgById(id) {
  return MSG_BY_ID[id] || MSG_BY_ID.noop;
}

function isSystem(msg) {
  return msg.status !== null && msg.status >= 0xF0;
}

// The named controllers worth recognising. Anything else is just a number.
const CC_NAMES = {
  0: 'Bank Select', 1: 'Modulation', 2: 'Breath', 4: 'Foot', 5: 'Portamento Time',
  6: 'Data Entry', 7: 'Volume', 8: 'Balance', 10: 'Pan', 11: 'Expression',
  12: 'Effect 1', 13: 'Effect 2', 64: 'Sustain', 65: 'Portamento', 66: 'Sostenuto',
  67: 'Soft Pedal', 68: 'Legato', 69: 'Hold 2', 71: 'Resonance', 72: 'Release Time',
  73: 'Attack Time', 74: 'Cutoff', 75: 'Decay Time', 91: 'Reverb', 92: 'Tremolo',
  93: 'Chorus', 94: 'Detune', 95: 'Phaser', 96: 'Data Increment', 97: 'Data Decrement',
  98: 'NRPN LSB', 99: 'NRPN MSB', 100: 'RPN LSB', 101: 'RPN MSB',
  120: 'All Sound Off', 121: 'Reset All Controllers', 122: 'Local Control',
  123: 'All Notes Off', 124: 'Omni Off', 125: 'Omni On', 126: 'Mono On', 127: 'Poly On',
};

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

function noteToken(value) {
  return NOTE_NAMES[value % 12] + '_' + (Math.floor(value / 12) - 1);
}

function noteLabel(value) {
  return NOTE_NAMES[value % 12] + (Math.floor(value / 12) - 1);
}

function parseNoteToken(text) {
  const match = /^([a-g]#?)_(-?\d+)$/i.exec(text);
  if (!match) return null;
  const index = NOTE_NAMES.indexOf(match[1].toUpperCase());
  if (index < 0) return null;
  return clamp((Number(match[2]) + 1) * 12 + index, 0, 127);
}

/* ------------------------------------------------------------------- utils */

function clamp(value, low, high) {
  return Math.min(high, Math.max(low, value));
}

function hex(value) {
  return '0x' + value.toString(16).toUpperCase();
}

function hex2(value) {
  return value.toString(16).toUpperCase().padStart(2, '0');
}

function parseHexValue(text, fallback) {
  const value = parseInt(text, 16);
  return Number.isNaN(value) ? fallback : value;
}

function h(tag, attrs, ...kids) {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs || {})) {
    if (value === null || value === undefined || value === false) continue;
    if (key === 'class') node.className = value;
    else if (key === 'text') node.textContent = value;
    else if (key === 'html') node.innerHTML = value;
    else if (key.startsWith('on')) node.addEventListener(key.slice(2), value);
    else node.setAttribute(key, value === true ? '' : value);
  }
  for (const kid of kids.flat()) {
    if (kid === null || kid === undefined || kid === false) continue;
    node.append(kid.nodeType ? kid : document.createTextNode(String(kid)));
  }
  return node;
}

function select(options, value, onchange, cls) {
  const node = h('select', { class: cls || '', onchange: (e) => onchange(e.target.value) });
  let group = null;
  let groupName = null;
  for (const option of options) {
    const item = h('option', { value: option.value }, option.label);
    if (String(option.value) === String(value)) item.selected = true;
    if (option.group && option.group !== groupName) {
      groupName = option.group;
      group = h('optgroup', { label: groupName });
      node.append(group);
    }
    (option.group ? group : node).append(item);
  }
  return node;
}

function field(label, control, sub) {
  return h('div', { class: 'field' },
    h('label', {}, label),
    control,
    sub === undefined ? null : h('span', { class: 'sub' }, sub));
}

/* ------------------------------------------------------------------- state */

function newOverride() {
  return { mode: 'default', value: 0 };
}

function newDevice() {
  return {
    type: 'default',
    channel: 'inherit',
    onChange: 'noop',
    onStop: 'noop',
    data: 0,
    ov: { change0: newOverride(), change1: newOverride(), stop0: newOverride(), stop1: newOverride() },
  };
}

function newModule() {
  return {
    channel: 0,
    velocity: 0x7F,
    alpha: 0.3,
    devices: Array.from({ length: DEVICE_COUNT }, newDevice),
  };
}

function newState() {
  return { modules: Array.from({ length: MODULE_COUNT }, newModule) };
}

// Matches EXAMPLE_CONFIG in main/configManager.cpp, so a first visit shows the
// same layout the device writes to itself on first boot.
const STARTER_CONFIG = `
[module1]
channel = 0x0
press_velocity = 0x7F
alpha = 0.5
[module1.device1]
message_on_change = note_on
message_on_stop = note_off
data = C_3
[module1.device2]
message_on_change = note_on
message_on_stop = note_off
data = D_3
[module1.device3]
message_on_change = note_on
message_on_stop = note_off
data = E_3
[module1.device4]
message_on_change = start
message_on_stop = stop
[module1.device5]
message_on_change = pc
data = 0x0

[module2]
channel = 0x0
alpha = 0.3
[module2.device1]
message_on_change = cc
data = 0x1
[module2.device2]
message_on_change = cc
data = 0x7
[module2.device3]
message_on_change = cc
data = 0xA
[module2.device4]
message_on_change = pitch_bend
[module2.device5]
message_on_change = channel_aftertouch

[module3]
channel = 0x1
alpha = 0.3
[module3.device1]
message_on_change = cc
data = 0x0
[module3.device2]
message_on_change = cc
data = 0x2
[module3.device3]
message_on_change = poly_aftertouch
data = C_3
[module3.device4]
message_on_change = cc
data = 0x4
[module3.device5]
message_on_change = cc
data = 0x5
`;

let state = newState();
let activeModule = 0;

const OVERRIDE_KEYS = [
  ['change0', 'manual_data_change_0'],
  ['change1', 'manual_data_change_1'],
  ['stop0', 'manual_data_stop_0'],
  ['stop1', 'manual_data_stop_1'],
];

function channelOf(module, device) {
  return device.channel === 'inherit' ? module.channel : Number(device.channel);
}

// Which kind of number the "data" option holds, taken from whichever
// configured message actually uses it.
function dataKindOf(device) {
  const change = msgById(device.onChange).dataKind;
  if (change !== 'none') return change;
  return msgById(device.onStop).dataKind;
}

function deviceIsSet(device) {
  return device.onChange !== 'noop' || device.onStop !== 'noop' ||
    device.type !== 'default' || device.channel !== 'inherit' ||
    OVERRIDE_KEYS.some(([key]) => device.ov[key].mode !== 'default');
}

function overrideCount(device) {
  return OVERRIDE_KEYS.filter(([key]) => device.ov[key].mode !== 'default').length;
}

// The bytes this device puts on the wire, mirroring MidiManager::sendMidiMsg.
function bytesFor(module, device, which) {
  const msg = msgById(which === 'change' ? device.onChange : device.onStop);
  if (msg.status === null) return null;

  const status = isSystem(msg) ? msg.status : (msg.status | channelOf(module, device));
  const out = [{ text: hex2(status), fixed: true }];
  const overrides = which === 'change'
    ? [device.ov.change0, device.ov.change1]
    : [device.ov.stop0, device.ov.stop1];

  [msg.d0, msg.d1].slice(0, msg.len - 1).forEach((slot, index) => {
    const override = overrides[index];
    if (override.mode === 'dev') return out.push({ text: 'vv', fixed: false });
    if (override.mode === 'fixed') return out.push({ text: hex2(override.value), fixed: true });

    switch (slot.src) {
      case 'data': return out.push({ text: hex2(device.data), fixed: true });
      case 'velocity': return out.push({ text: hex2(module.velocity), fixed: true });
      case 'lsb': return out.push({ text: 'll', fixed: false });
      case 'msb': return out.push({ text: 'mm', fixed: false });
      default: return out.push({ text: 'vv', fixed: false });
    }
  });

  return out;
}

/* ---------------------------------------------------------------- presets */

// One per module type. The three boards each present five inputs to the
// firmware, but the slider module wires two faders across them: inputs 2 and 4
// mirror 1 and 3, and input 5 sits at ground.
const PRESETS = [
  {
    id: 'buttons',
    label: 'Buttons',
    title: 'Button module — five keys sending note on / note off',
    apply(module, index) {
      // Buttons come in through the ADC like everything else, so the smoothing
      // has to be loose or a press reads as a slow ramp.
      module.alpha = 0.5;
      module.devices.forEach((device, i) => {
        Object.assign(device, newDevice(), {
          onChange: 'note_on',
          onStop: 'note_off',
          data: clamp(48 + index * DEVICE_COUNT + i, 0, 127),
        });
      });
    },
  },
  {
    id: 'sliders',
    label: 'Sliders',
    title: 'Slider module — two faders on inputs 1 and 3; 2 and 4 mirror them, 5 is tied to ground',
    apply(module, index) {
      module.alpha = 0.3;
      module.devices.forEach((device) => Object.assign(device, newDevice()));
      Object.assign(module.devices[0], { onChange: 'cc', data: clamp(7 + index * 2, 0, 127) });
      Object.assign(module.devices[2], { onChange: 'cc', data: clamp(8 + index * 2, 0, 127) });
    },
  },
  {
    id: 'pots',
    label: 'Pots',
    title: 'Pot module — five knobs sending control change',
    apply(module, index) {
      module.alpha = 0.3;
      module.devices.forEach((device, i) => {
        Object.assign(device, newDevice(), {
          onChange: 'cc',
          data: clamp(20 + index * DEVICE_COUNT + i, 0, 127),
        });
      });
    },
  },
  {
    id: 'clear',
    label: 'Clear',
    title: 'Send nothing from this module',
    apply(module) {
      module.devices.forEach((device) => Object.assign(device, newDevice()));
    },
  },
];

/* ------------------------------------------------------------------ render */

const messageOptions = MESSAGES.map((m) => ({ value: m.id, label: m.label, group: m.group }));
const channelOptions = Array.from({ length: 16 }, (_, i) => ({ value: i, label: 'Channel ' + (i + 1) }));
const deviceChannelOptions = [{ value: 'inherit', label: 'Module channel' }].concat(channelOptions);
const noteOptions = NOTE_NAMES.map((name, i) => ({ value: i, label: name }));
const octaveOptions = Array.from({ length: 11 }, (_, i) => ({ value: i - 1, label: String(i - 1) }));

const tabsEl = document.getElementById('tabs');
const panelsEl = document.getElementById('panels');
const tomlEl = document.getElementById('toml-out');
const tomlMetaEl = document.getElementById('toml-meta');

function render() {
  tabsEl.replaceChildren();
  panelsEl.replaceChildren();

  state.modules.forEach((module, index) => {
    const tab = h('button', {
      type: 'button',
      class: 'tab',
      role: 'tab',
      'aria-selected': String(index === activeModule),
      onclick: () => { activeModule = index; render(); },
    }, h('strong', {}, 'Module ' + (index + 1)), h('span', {}, summariseModule(module)));
    tabsEl.append(tab);

    const panel = h('div', { class: 'panel' + (index === activeModule ? ' active' : '') },
      renderModuleBar(module, index),
      h('div', { class: 'devices' }, module.devices.map((device, i) => renderDevice(module, device, i))));
    panelsEl.append(panel);
  });

  updateToml();
}

function summariseModule(module) {
  const active = module.devices.filter(deviceIsSet).length;
  if (!active) return 'silent';
  return active + ' of ' + DEVICE_COUNT + ' inputs';
}

function renderModuleBar(module, index) {
  const alphaSub = h('span', { class: 'sub' }, module.alpha.toFixed(2));
  const alpha = h('input', {
    type: 'range', min: '0.01', max: '0.5', step: '0.01', value: String(module.alpha),
    oninput: (e) => {
      module.alpha = Number(e.target.value);
      alphaSub.textContent = module.alpha.toFixed(2);
      updateToml();
    },
  });

  const velocitySub = h('span', { class: 'sub' }, hex(module.velocity));
  const velocity = h('input', {
    type: 'number', min: '0', max: '127', value: String(module.velocity),
    oninput: (e) => {
      module.velocity = clamp(Number(e.target.value) || 0, 0, 127);
      velocitySub.textContent = hex(module.velocity);
      refreshPanel(index);
    },
  });

  return h('div', { class: 'module-bar' },
    field('Channel', select(channelOptions, module.channel, (value) => {
      module.channel = Number(value);
      refreshPanel(index);
    }), hex(module.channel)),
    h('div', { class: 'field' }, h('label', {}, 'Press velocity'), velocity, velocitySub),
    h('div', { class: 'field' }, h('label', {}, 'Alpha (smoothing)'), alpha, alphaSub),
    h('div', { class: 'presets' }, PRESETS.map((preset) => h('button', {
      type: 'button', class: 'btn small ghost', title: preset.title,
      onclick: () => { preset.apply(module, index); render(); },
    }, preset.label))));
}

function renderDevice(module, device, index) {
  const card = h('div', { class: 'device' });
  const wire = h('div', { class: 'wire' });
  const dataField = h('div', { class: 'field data-field' });
  const dataLabel = h('label', {}, 'Data');
  const dataSub = h('span', { class: 'sub' });

  const noteRow = h('div', { class: 'override' },
    select(noteOptions, device.data % 12, (value) => {
      device.data = clamp(Math.floor(device.data / 12) * 12 + Number(value), 0, 127);
      refresh();
    }, 'small'),
    select(octaveOptions, Math.floor(device.data / 12) - 1, (value) => {
      device.data = clamp((Number(value) + 1) * 12 + (device.data % 12), 0, 127);
      refresh();
    }, 'small'));

  const numberInput = h('input', {
    type: 'number', min: '0', max: '127', value: String(device.data),
    oninput: (e) => { device.data = clamp(Number(e.target.value) || 0, 0, 127); refresh(); },
  });

  dataField.append(dataLabel, noteRow, numberInput, dataSub);

  const summary = h('summary', {}, 'Manual data bytes', h('span', { class: 'badge' }));
  const advanced = h('details', { class: 'advanced' }, summary,
    h('div', { class: 'overrides' }, OVERRIDE_KEYS.map(([key, name]) => renderOverride(device, key, name, refresh))));

  card.append(
    h('div', { class: 'device-head' },
      h('span', { class: 'device-index' }, 'Device ' + (index + 1)),
      wire),
    h('div', { class: 'device-grid' },
      field('On change', select(messageOptions, device.onChange, (value) => { device.onChange = value; refresh(); })),
      field('On stop', select(messageOptions, device.onStop, (value) => { device.onStop = value; refresh(); })),
      dataField,
      field('Reads as', select([
        { value: 'default', label: 'Module default' },
        { value: 'analog', label: 'Analog (0-127)' },
        { value: 'digital', label: 'Digital (edges)' },
      ], device.type, (value) => { device.type = value; refresh(); })),
      field('Channel', select(deviceChannelOptions, device.channel, (value) => {
        device.channel = value === 'inherit' ? 'inherit' : Number(value);
        refresh();
      }))),
    advanced);

  function refresh() {
    const kind = dataKindOf(device);
    const set = deviceIsSet(device);
    card.classList.toggle('is-off', !set);

    dataField.classList.toggle('empty', kind === 'none');
    noteRow.hidden = kind !== 'note';
    numberInput.hidden = kind === 'note';

    if (kind === 'note') {
      dataLabel.textContent = 'Note';
      dataSub.textContent = noteLabel(device.data) + ' · ' + device.data;
    } else if (kind === 'cc') {
      dataLabel.textContent = 'Controller';
      numberInput.value = String(device.data);
      dataSub.textContent = CC_NAMES[device.data] ? 'CC ' + device.data + ' · ' + CC_NAMES[device.data] : 'CC ' + device.data;
    } else if (kind === 'program') {
      dataLabel.textContent = 'Program';
      numberInput.value = String(device.data);
      dataSub.textContent = hex(device.data) + ' · DAW program ' + (device.data + 1);
    } else if (kind === 'raw') {
      dataLabel.textContent = 'Value';
      numberInput.value = String(device.data);
      dataSub.textContent = hex(device.data);
    }

    const count = overrideCount(device);
    const badge = summary.querySelector('.badge');
    badge.textContent = count ? String(count) : '';
    badge.hidden = !count;

    renderWire(wire, module, device);
    updateToml();
  }

  refresh();
  return card;
}

function renderOverride(device, key, name, refresh) {
  const override = device.ov[key];
  const value = h('input', {
    type: 'number', min: '0', max: '127', value: String(override.value),
    oninput: (e) => { override.value = clamp(Number(e.target.value) || 0, 0, 127); refresh(); },
  });
  value.disabled = override.mode !== 'fixed';

  const mode = select([
    { value: 'default', label: 'Default' },
    { value: 'dev', label: 'Input value' },
    { value: 'fixed', label: 'Fixed' },
  ], override.mode, (next) => {
    override.mode = next;
    value.disabled = next !== 'fixed';
    refresh();
  }, 'small');

  return h('div', { class: 'override' }, h('label', {}, name), mode, value);
}

function renderWire(node, module, device) {
  node.replaceChildren();
  for (const which of ['change', 'stop']) {
    const bytes = bytesFor(module, device, which);
    if (!bytes) continue;
    node.append(h('span', { class: 'w' },
      h('i', {}, which === 'change' ? 'change' : 'stop'),
      bytes.map((byte) => h(byte.fixed ? 'b' : 'i', {}, byte.text + ' '))));
  }
  if (!node.childNodes.length) node.append(h('i', {}, 'sends nothing'));
}

// Redraw the module currently on screen, e.g. after a channel change that
// every device's byte preview depends on.
function refreshPanel(index) {
  if (index === activeModule) render();
  else updateToml();
}

/* -------------------------------------------------------------------- toml */

function toToml() {
  const lines = [
    '# MIDI-Mod configuration',
    '# stephenbdennis.github.io/midi_mod',
    '# Send to the device over Bluetooth, or keep as config.toml.',
  ];

  state.modules.forEach((module, mi) => {
    lines.push('', '[module' + (mi + 1) + ']');
    lines.push('channel = ' + hex(module.channel));
    lines.push('press_velocity = ' + hex(module.velocity));
    lines.push('alpha = ' + module.alpha.toFixed(2));

    module.devices.forEach((device, di) => {
      if (!deviceIsSet(device)) return;
      lines.push('[module' + (mi + 1) + '.device' + (di + 1) + ']');
      if (device.type !== 'default') lines.push('device_type = ' + device.type);
      if (device.channel !== 'inherit') lines.push('channel = ' + hex(Number(device.channel)));
      if (device.onChange !== 'noop') lines.push('message_on_change = ' + device.onChange);
      if (device.onStop !== 'noop') lines.push('message_on_stop = ' + device.onStop);

      const kind = dataKindOf(device);
      if (kind !== 'none') lines.push('data = ' + (kind === 'note' ? noteToken(device.data) : hex(device.data)));

      for (const [key, name] of OVERRIDE_KEYS) {
        const override = device.ov[key];
        if (override.mode === 'dev') lines.push(name + ' = dev');
        else if (override.mode === 'fixed') lines.push(name + ' = ' + hex(override.value));
      }
    });
  });

  return lines.join('\n') + '\n';
}

function updateToml() {
  const text = toToml();
  tomlEl.replaceChildren();
  for (const line of text.split('\n')) {
    const cls = line.startsWith('#') ? 'c' : line.startsWith('[') ? 's' : 'k';
    tomlEl.append(h('span', { class: cls }, line), '\n');
  }
  const active = state.modules.reduce((n, m) => n + m.devices.filter(deviceIsSet).length, 0);
  tomlMetaEl.textContent = active + '/' + (MODULE_COUNT * DEVICE_COUNT) + ' inputs · ' + text.length + ' B';
  save();
}

function fromToml(text) {
  const next = newState();
  let moduleIndex = 0;
  let deviceIndex = 0;
  let inDevice = false;

  for (const raw of text.split(/\r?\n/)) {
    const line = raw.replace(/[ \t]/g, '');
    if (!line || line.startsWith('#')) continue;

    if (line.startsWith('[')) {
      const close = line.indexOf(']');
      const parts = line.slice(1, close === -1 ? undefined : close).split('.');
      const module = /^module([1-3])$/i.exec(parts[0]);
      if (module) {
        moduleIndex = Number(module[1]) - 1;
        deviceIndex = 0;
        inDevice = false;
      }
      const device = parts[1] && /^device([1-5])$/i.exec(parts[1]);
      if (device) {
        deviceIndex = Number(device[1]) - 1;
        inDevice = true;
      }
      continue;
    }

    const equals = line.indexOf('=');
    if (equals === -1) continue;
    applySetting(next.modules[moduleIndex], deviceIndex, inDevice,
      line.slice(0, equals).toLowerCase(), line.slice(equals + 1));
  }

  return next;
}

function applySetting(module, deviceIndex, inDevice, key, value) {
  const device = module.devices[deviceIndex];

  switch (key) {
    case 'channel':
      if (inDevice) device.channel = /^inherit|module$/i.test(value) ? 'inherit' : clamp(parseHexValue(value, 0), 0, 15);
      else module.channel = clamp(parseHexValue(value, 0), 0, 15);
      return;
    case 'press_velocity':
      module.velocity = clamp(parseHexValue(value, 127), 0, 127);
      return;
    case 'alpha':
      module.alpha = clamp(Number(value) || 0.3, 0.01, 0.5);
      return;
    case 'device_type':
      device.type = /^analog$/i.test(value) ? 'analog' : /^digital$/i.test(value) ? 'digital' : 'default';
      return;
    case 'message_on_change':
      device.onChange = messageIdFromToken(value);
      return;
    case 'message_on_stop':
      device.onStop = messageIdFromToken(value);
      return;
    case 'data':
      device.data = parseNoteToken(value) ?? clamp(parseHexValue(value, 0), 0, 127);
      return;
    default: {
      const entry = OVERRIDE_KEYS.find(([, name]) => name === key);
      if (!entry) return;
      const override = device.ov[entry[0]];
      if (/^dev$/i.test(value)) override.mode = 'dev';
      else {
        override.mode = 'fixed';
        override.value = clamp(parseHexValue(value, 0), 0, 127);
      }
    }
  }
}

function messageIdFromToken(token) {
  const name = token.toLowerCase();
  if (MSG_BY_ID[name]) return name;
  if (ALIASES[name]) return ALIASES[name];

  // Raw status byte. The firmware keeps only the high nibble of channel
  // voice messages and takes the channel from the config.
  const status = parseHexValue(name, NaN);
  if (Number.isNaN(status)) return 'noop';
  const base = status >= 0xF0 ? status : status & 0xF0;
  const match = MESSAGES.find((m) => m.status === base);
  return match ? match.id : 'noop';
}

/* ---------------------------------------------------------------- storage */

let saveTimer = null;

function save() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => {
    try {
      localStorage.setItem(STORE_KEY, JSON.stringify(state));
    } catch (err) {
      /* private browsing, quota, or storage disabled - nothing to do */
    }
  }, 250);
}

function load() {
  try {
    const stored = localStorage.getItem(STORE_KEY);
    if (!stored) {
      state = fromToml(STARTER_CONFIG);
      return;
    }
    // Round-trip through the generator so an old or hand-edited entry cannot
    // leave the editor holding a shape it does not understand.
    state = fromToml(tomlFromStored(JSON.parse(stored)));
  } catch (err) {
    state = fromToml(STARTER_CONFIG);
  }
}

function tomlFromStored(stored) {
  const previous = state;
  state = stored;
  const text = toToml();
  state = previous;
  return text;
}

/* --------------------------------------------------------------- toolbar */

const toastEl = document.getElementById('toast');
let toastTimer = null;

function toast(message) {
  toastEl.textContent = message;
  toastEl.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toastEl.classList.remove('show'), 1800);
}

document.getElementById('download-btn').addEventListener('click', () => {
  const blob = new Blob([toToml()], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);
  const link = h('a', { href: url, download: 'config.toml' });
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
});

document.getElementById('copy-btn').addEventListener('click', async () => {
  const text = toToml();
  try {
    await navigator.clipboard.writeText(text);
    toast('Copied');
  } catch (err) {
    const area = h('textarea', { style: 'position:fixed;opacity:0' });
    area.value = text;
    document.body.append(area);
    area.select();
    document.execCommand('copy');
    area.remove();
    toast('Copied');
  }
});

const fileInput = document.getElementById('import-file');
document.getElementById('import-btn').addEventListener('click', () => fileInput.click());
fileInput.addEventListener('change', () => {
  const file = fileInput.files && fileInput.files[0];
  if (file) readConfigFile(file);
  fileInput.value = '';
});

function readConfigFile(file) {
  file.text().then((text) => {
    state = fromToml(text);
    render();
    toast('Loaded ' + file.name);
  });
}

document.addEventListener('dragover', (e) => e.preventDefault());
document.addEventListener('drop', (e) => {
  const file = e.dataTransfer && e.dataTransfer.files[0];
  if (!file) return;
  e.preventDefault();
  readConfigFile(file);
});

const themeButton = document.getElementById('theme-toggle');
const storedTheme = (() => {
  try { return localStorage.getItem('midimod.theme'); } catch (err) { return null; }
})();

function setTheme(theme) {
  document.documentElement.dataset.theme = theme;
  try { localStorage.setItem('midimod.theme', theme); } catch (err) { /* ignore */ }
}

setTheme(storedTheme || (matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark'));
themeButton.addEventListener('click', () => {
  setTheme(document.documentElement.dataset.theme === 'light' ? 'dark' : 'light');
});

/* ------------------------------------------------------------ device / BLE */

/* The device used to mount as a USB drive in a special config boot mode. It
 * doesn't any more: USB is the MIDI interface full time, and config.toml moves
 * over a GATT service instead, so the device can be reconfigured without ever
 * leaving the DAW.
 *
 * Every constant below mirrors main/configProtocol.hpp. Web Bluetooth is
 * Chromium-only and needs HTTPS, which GitHub Pages provides. */

const BLE_SERVICE = '6d696469-2d6d-6f64-0001-636f6e666967';
const BLE_CONTROL = '6d696469-2d6d-6f64-0003-636f6e666967';
const BLE_DATA    = '6d696469-2d6d-6f64-0004-636f6e666967';

const CMD_HELLO = 0x00;
const CMD_READ = 0x01;
const CMD_WRITE = 0x02;
const CMD_COMMIT = 0x03;

const EVT_READ_BEGIN = 0x81;
const EVT_READ_END = 0x82;
const EVT_WRITE_READY = 0x83;
const EVT_WRITE_OK = 0x84;
const EVT_ERR = 0x85;
const EVT_HELLO = 0x86;

const BLE_ERRORS = {
  0x01: 'the config is larger than the device will accept',
  0x02: 'the checksum did not match, so the transfer was corrupted',
  0x03: 'the device was not expecting config data',
  0x04: 'the device could not write the file',
  0x05: 'the transfer length did not match',
  0x06: 'the device did not understand the command',
};

/* CRC-16/CCITT-FALSE, the same four lines as configCrc16 in the firmware. */
function crc16(bytes) {
  let crc = 0xFFFF;

  for (const byte of bytes) {
    crc ^= byte << 8;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) & 0xFFFF : (crc << 1) & 0xFFFF;
    }
  }

  return crc;
}

const bleSend = document.getElementById('ble-send');
const bleConnectButton = document.getElementById('ble-connect');
const bleLoadButton = document.getElementById('ble-load');
const bleDisconnectButton = document.getElementById('ble-disconnect');
const bleMeta = document.getElementById('ble-meta');
const bleHint = document.getElementById('ble-hint');

const canBle = !!(navigator.bluetooth && navigator.bluetooth.requestDevice);

let bleDevice = null;
let bleControl = null;
let bleData = null;
let bleMtu = 23;
let bleMax = 4096;
let bleBusy = false;

/* One command is in flight at a time, so a single slot is enough to match a
 * reply to whoever asked for it. */
let blePending = null;

/* A read spans two control events with the data chunks in between, so the
 * session is set up before the command goes out and torn down by whichever
 * event ends it. */
let bleRead = null;

function setBleState(text, message, tone) {
  bleMeta.textContent = text;
  bleHint.className = 'hint' + (tone ? ' ' + tone : '');
  bleHint.replaceChildren();
  bleHint.append(message);
}

function bleConnected() {
  return !!(bleDevice && bleDevice.gatt && bleDevice.gatt.connected);
}

function refreshBleButtons() {
  const ready = bleConnected() && !bleBusy;
  bleSend.disabled = !ready;
  bleLoadButton.disabled = !ready;
  bleConnectButton.hidden = bleConnected();
  bleDisconnectButton.hidden = !bleConnected();
  bleConnectButton.disabled = bleBusy || !canBle;
}

function describeBleIdle() {
  if (!canBle) {
    setBleState('unsupported',
      'This browser has no Web Bluetooth. Chrome, Edge and Opera do; on Linux you may need to enable it in chrome://flags. You can still download config.toml.',
      'warn');
    return;
  }

  if (bleConnected()) {
    setBleState('connected', 'Connected to ' + (bleDevice.name || 'the device') + '. Send writes config.toml and applies it straight away.');
  } else {
    setBleState('not connected', 'Connect over Bluetooth to read the config that is on the device now, or to send it a new one.');
  }
}

function u16(value) {
  return [value & 0xFF, (value >> 8) & 0xFF];
}

function sendCommand(command, args = []) {
  return bleControl.writeValueWithResponse(new Uint8Array([command, ...args]));
}

/* Install the waiter before the command goes out, so a fast reply cannot land
 * before anything is listening for it. */
function request(want, send, timeout = 8000) {
  return new Promise((resolve, reject) => {
    const pending = { want, resolve, reject };
    pending.timer = setTimeout(() => {
      if (blePending === pending) blePending = null;
      reject(new Error('the device did not answer in time'));
    }, timeout);

    blePending = pending;

    Promise.resolve().then(send).catch((err) => {
      clearTimeout(pending.timer);
      if (blePending === pending) blePending = null;
      reject(err);
    });
  });
}

function failPending(error) {
  if (blePending) {
    clearTimeout(blePending.timer);
    blePending.reject(error);
    blePending = null;
  }

  if (bleRead) {
    clearTimeout(bleRead.timer);
    bleRead.reject(error);
    bleRead = null;
  }
}

function onControlNotify(event) {
  const view = event.target.value;
  const code = view.getUint8(0);

  if (code === EVT_ERR) {
    const raw = view.byteLength > 1 ? view.getUint8(1) : 0;
    failPending(new Error(BLE_ERRORS[raw] || ('device error 0x' + raw.toString(16))));
    return;
  }

  if (code === EVT_READ_BEGIN && bleRead) {
    bleRead.expected = view.getUint16(1, true);
    return;
  }

  if (code === EVT_READ_END && bleRead) {
    const session = bleRead;
    bleRead = null;
    clearTimeout(session.timer);

    const bytes = new Uint8Array(session.received);
    if (session.expected >= 0 && bytes.length !== session.expected) {
      session.reject(new Error('the device sent ' + bytes.length + ' bytes but announced ' + session.expected));
      return;
    }

    if (crc16(bytes) !== view.getUint16(1, true)) {
      session.reject(new Error('the config arrived corrupted'));
      return;
    }

    session.resolve(new TextDecoder().decode(bytes));
    return;
  }

  if (blePending && blePending.want.includes(code)) {
    const pending = blePending;
    blePending = null;
    clearTimeout(pending.timer);
    pending.resolve(view);
  }
}

function onDataNotify(event) {
  if (!bleRead) return;

  const view = event.target.value;
  for (let i = 0; i < view.byteLength; i += 1) {
    bleRead.received.push(view.getUint8(i));
  }
}

function onBleDisconnected() {
  failPending(new Error('the device disconnected'));
  bleControl = null;
  bleData = null;
  bleBusy = false;
  refreshBleButtons();
  setBleState('disconnected', 'The device went away. Connect again to carry on.', 'warn');
}

async function bleConnect() {
  bleDevice = await navigator.bluetooth.requestDevice({
    filters: [{ services: [BLE_SERVICE] }],
    optionalServices: [BLE_SERVICE],
  });

  bleDevice.addEventListener('gattserverdisconnected', onBleDisconnected);

  const server = await bleDevice.gatt.connect();
  const service = await server.getPrimaryService(BLE_SERVICE);

  bleControl = await service.getCharacteristic(BLE_CONTROL);
  bleData = await service.getCharacteristic(BLE_DATA);

  bleControl.addEventListener('characteristicvaluechanged', onControlNotify);
  await bleControl.startNotifications();
  bleData.addEventListener('characteristicvaluechanged', onDataNotify);
  await bleData.startNotifications();

  // The browser never exposes the negotiated MTU, so the device reports it.
  const hello = await request([EVT_HELLO], () => sendCommand(CMD_HELLO));
  bleMtu = hello.getUint16(2, true);
  bleMax = hello.getUint16(4, true);
}

function readFromDevice() {
  return new Promise((resolve, reject) => {
    const session = { received: [], expected: -1, resolve, reject };
    session.timer = setTimeout(() => {
      if (bleRead === session) bleRead = null;
      reject(new Error('the device did not finish sending in time'));
    }, 15000);

    bleRead = session;

    Promise.resolve().then(() => sendCommand(CMD_READ)).catch((err) => {
      clearTimeout(session.timer);
      if (bleRead === session) bleRead = null;
      reject(err);
    });
  });
}

async function writeToDevice(text) {
  const bytes = new TextEncoder().encode(text);

  if (bytes.length > bleMax) {
    throw new Error('the config is ' + bytes.length + ' bytes, over the device limit of ' + bleMax);
  }

  await request([EVT_WRITE_READY], () => sendCommand(CMD_WRITE, u16(bytes.length)));

  // Three bytes of the MTU go to the ATT header, and Chrome caps a single
  // write at 512 bytes regardless.
  const chunk = Math.min(Math.max(bleMtu - 3, 20), 512);
  for (let offset = 0; offset < bytes.length; offset += chunk) {
    await bleData.writeValueWithResponse(bytes.slice(offset, offset + chunk));
  }

  await request([EVT_WRITE_OK], () => sendCommand(CMD_COMMIT, u16(crc16(bytes))), 15000);
}

async function withDevice(label, work) {
  if (!bleConnected() || bleBusy) return;

  bleBusy = true;
  refreshBleButtons();

  try {
    await work();
  } catch (err) {
    const reason = err && err.message ? err.message : String(err);
    setBleState('failed', label + ' failed: ' + reason, 'warn');
    toast(label + ' failed');
  } finally {
    bleBusy = false;
    refreshBleButtons();
  }
}

bleConnectButton.addEventListener('click', async () => {
  if (!canBle) return;

  bleConnectButton.disabled = true;

  try {
    await bleConnect();
    describeBleIdle();
    toast('Connected to ' + (bleDevice.name || 'the device'));
  } catch (err) {
    // Dismissing the chooser is a NotFoundError, not something worth shouting about.
    if (err && err.name === 'NotFoundError') {
      describeBleIdle();
    } else {
      const reason = err && err.message ? err.message : String(err);
      setBleState('failed', 'Could not connect: ' + reason, 'warn');
    }
  } finally {
    refreshBleButtons();
  }
});

bleDisconnectButton.addEventListener('click', () => {
  if (bleDevice && bleDevice.gatt && bleDevice.gatt.connected) {
    bleDevice.gatt.disconnect();
  }
});

bleLoadButton.addEventListener('click', () => withDevice('Load', async () => {
  const text = await readFromDevice();

  if (!text.trim()) {
    setBleState('empty', 'The device has no config on it yet. Send one to get started.', 'warn');
    toast('Device config is empty');
    return;
  }

  state = fromToml(text);
  render();

  const time = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  setBleState('loaded ' + time, 'Loaded ' + text.length + ' bytes from the device.');
  toast('Loaded from device');
}));

bleSend.addEventListener('click', () => withDevice('Send', async () => {
  await writeToDevice(toToml());

  const time = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  setBleState('sent ' + time, 'config.toml is on the device and already in effect.');
  toast('Sent to device');
}));

describeBleIdle();
refreshBleButtons();

/* ---------------------------------------------------------- midi monitor */

const monitorMeta = document.getElementById('monitor-meta');
const monitorHint = document.getElementById('monitor-hint');
const logEl = document.getElementById('midi-log');
const inputSelect = document.getElementById('midi-input');
const connectButton = document.getElementById('midi-connect');

let midiAccess = null;
let midiPort = null;
let lastRow = null;
let lastKey = '';

document.getElementById('midi-clear').addEventListener('click', () => {
  logEl.replaceChildren();
  lastRow = null;
  lastKey = '';
});

if (!navigator.requestMIDIAccess) {
  connectButton.disabled = true;
  monitorHint.textContent = 'This browser has no Web MIDI support. Chrome, Edge and Opera do.';
}

connectButton.addEventListener('click', async () => {
  try {
    midiAccess = await navigator.requestMIDIAccess();
  } catch (err) {
    monitorMeta.textContent = 'blocked';
    monitorHint.textContent = 'MIDI access was denied. Allow it in the browser site settings and try again.';
    return;
  }

  connectButton.hidden = true;
  inputSelect.disabled = false;
  midiAccess.onstatechange = listInputs;
  listInputs();
});

function listInputs() {
  const inputs = [...midiAccess.inputs.values()];
  inputSelect.replaceChildren();

  if (!inputs.length) {
    inputSelect.append(h('option', {}, 'No MIDI inputs'));
    inputSelect.disabled = true;
    monitorMeta.textContent = 'no inputs';
    monitorHint.textContent = 'Plug the device in over USB and it appears here as a MIDI input.';
    return;
  }

  inputSelect.disabled = false;
  const preferred = inputs.find((input) => /midi.?mod/i.test(input.name || '')) || inputs[0];
  for (const input of inputs) {
    const option = h('option', { value: input.id }, input.name || input.id);
    if (input === preferred) option.selected = true;
    inputSelect.append(option);
  }
  listen(preferred.id);
}

inputSelect.addEventListener('change', () => listen(inputSelect.value));

function listen(id) {
  for (const input of midiAccess.inputs.values()) input.onmidimessage = null;
  midiPort = midiAccess.inputs.get(id);
  if (!midiPort) return;
  midiPort.onmidimessage = onMidiMessage;
  monitorMeta.textContent = 'listening';
  monitorHint.textContent = 'Move an input on the device to see what it sends.';
}

function onMidiMessage(event) {
  const data = [...event.data];
  const bytes = data.map(hex2).join(' ');
  const decoded = decode(data);
  const key = bytes;

  if (key === lastKey && lastRow) {
    const repeat = lastRow.querySelector('.repeat');
    repeat.textContent = '×' + (Number(repeat.dataset.n || 1) + 1);
    repeat.dataset.n = String(Number(repeat.dataset.n || 1) + 1);
    return;
  }

  const row = h('li', {},
    h('span', { class: 'bytes' }, bytes),
    h('span', { class: 'name' }, decoded.name),
    h('span', { class: 'ch' }, decoded.detail),
    h('span', { class: 'ch repeat', 'data-n': '1' }, ''));

  logEl.prepend(row);
  while (logEl.childElementCount > 200) logEl.lastElementChild.remove();
  lastRow = row;
  lastKey = key;
}

function decode(data) {
  const status = data[0];
  if (status < 0x80) return { name: 'data', detail: '' };

  const system = status >= 0xF0;
  const base = system ? status : status & 0xF0;
  const msg = MESSAGES.find((m) => m.status === base);
  if (!msg) return { name: 'unknown', detail: '' };

  const channel = system ? '' : 'ch ' + ((status & 0x0F) + 1);
  let detail = channel;

  if (msg.id === 'note_on' || msg.id === 'note_off' || msg.id === 'poly_aftertouch') {
    detail = noteLabel(data[1]) + (channel ? ' · ' + channel : '');
  } else if (msg.id === 'cc') {
    const name = CC_NAMES[data[1]];
    detail = 'CC ' + data[1] + (name ? ' ' + name : '') + ' = ' + data[2] + (channel ? ' · ' + channel : '');
  } else if (msg.id === 'pc') {
    detail = 'program ' + data[1] + (channel ? ' · ' + channel : '');
  } else if (msg.id === 'pitch_bend' || msg.id === 'song_position') {
    detail = String(((data[2] || 0) << 7) | (data[1] || 0)) + (channel ? ' · ' + channel : '');
  } else if (msg.id === 'channel_aftertouch') {
    detail = data[1] + (channel ? ' · ' + channel : '');
  }

  return { name: msg.label, detail };
}

/* -------------------------------------------------------------- reference */

(function renderReference() {
  const body = document.querySelector('#reference-table tbody');
  let group = null;

  for (const msg of MESSAGES) {
    const first = msg.group !== group;
    group = msg.group;
    body.append(h('tr', { class: first ? 'group-start' : '' },
      h('td', {}, msg.label),
      h('td', { class: 'mono' }, msg.id),
      h('td', { class: 'mono' }, msg.status === null ? '—' : '0x' + hex2(msg.status)),
      h('td', { class: 'dim' }, String(msg.len)),
      h('td', { class: 'dim' }, describeSlot(msg.d0)),
      h('td', { class: 'dim' }, describeSlot(msg.d1))));
  }
})();

function describeSlot(slot) {
  if (!slot) return '—';
  const source = { data: 'data', velocity: 'press_velocity', input: 'input', lsb: 'input', msb: 'input' }[slot.src];
  return slot.label + ' (' + source + ')';
}

/* ------------------------------------------------------------------- boot */

load();
render();
