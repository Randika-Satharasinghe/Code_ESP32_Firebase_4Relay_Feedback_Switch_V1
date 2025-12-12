import { initializeApp } from "https://www.gstatic.com/firebasejs/9.22.0/firebase-app.js";
import { getDatabase, ref, onValue, set } from "https://www.gstatic.com/firebasejs/9.22.0/firebase-database.js";

// Firebase Config
const firebaseConfig = {
    apiKey: "AIzaSyACy6ePjitOl0YisxlPgPSij394OE2OcZg",
    databaseURL: "https://esp-32-relay-01-5bb14-default-rtdb.firebaseio.com/"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

// DOM Elements
const statusBadge = document.getElementById('connectionStatus');
const els = {
    temp: document.getElementById('tempValue'),
    hum: document.getElementById('humValue'),
    moist: document.getElementById('moistureValue'),
    light: document.getElementById('lightValue'),
    fanStatus: document.getElementById('fanStatus'),
    lightStatus: document.getElementById('lightStatus'),
    tempInput: document.getElementById('tempThresholdInput'),
    lightInput: document.getElementById('lightThresholdInput'),
    saveBtn: document.getElementById('saveConfigBtn')
};

// State
let currentThresholds = { temp: 30, light: 1000 };

// Chart.js Setup
const ctx = document.getElementById('sensorChart').getContext('2d');
const sensorChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            { label: 'Temp (°C)', borderColor: '#ff5252', data: [] },
            { label: 'Humidity (%)', borderColor: '#2979ff', data: [] },
            { label: 'Moisture', borderColor: '#00e676', data: [], hidden: true }, // Hidden by default to avoid scale messes
            { label: 'Light', borderColor: '#ffea00', data: [], hidden: true }
        ]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
            x: { grid: { color: 'rgba(255,255,255,0.1)' } },
            y: { grid: { color: 'rgba(255,255,255,0.1)' } }
        }
    }
});

// Update Chart
function updateChart(temp, hum, moist, light) {
    const now = new Date().toLocaleTimeString();
    
    if (sensorChart.data.labels.length > 20) {
        sensorChart.data.labels.shift();
        sensorChart.data.datasets.forEach(bs => bs.data.shift());
    }

    sensorChart.data.labels.push(now);
    sensorChart.data.datasets[0].data.push(temp);
    sensorChart.data.datasets[1].data.push(hum);
    sensorChart.data.datasets[2].data.push(moist);
    sensorChart.data.datasets[3].data.push(light);
    sensorChart.update();
}

// Listen to Status
const statusRef = ref(db, 'greenhouse/status');
onValue(statusRef, (snapshot) => {
    const data = snapshot.val();
    if (data) {
        statusBadge.textContent = "Connected";
        statusBadge.style.color = "#00e676";
        
        els.temp.textContent = data.temp?.toFixed(1) || "--";
        els.hum.textContent = data.humidity?.toFixed(1) || "--";
        els.moist.textContent = data.moisture || "--";
        els.light.textContent = data.light || "--";

        // Determine Actuator Status (Simulated logic based on values & thresholds)
        // Since ESP32 controls it, we ideally should read actuator status from Firebase.
        // But ESP32 doesn't write actuator status (only reads config).
        // So we infer it, or we could update ESP to write status.
        // For now, let's infer based on received values and known thresholds.
        
        // Fan
        if (data.temp > currentThresholds.temp) {
            els.fanStatus.textContent = "ON";
            els.fanStatus.className = "status-pill on";
        } else {
            els.fanStatus.textContent = "OFF";
            els.fanStatus.className = "status-pill off";
        }

        // Light
        if (data.light < currentThresholds.light) {
            els.lightStatus.textContent = "ON";
            els.lightStatus.className = "status-pill on";
        } else {
            els.lightStatus.textContent = "OFF";
            els.lightStatus.className = "status-pill off";
        }

        updateChart(data.temp, data.humidity, data.moisture, data.light);
    }
});

// Listen to Config (Initial Load)
const configRef = ref(db, 'greenhouse/config');
onValue(configRef, (snapshot) => {
    const data = snapshot.val();
    if (data) {
        if (data.tempThreshold) {
            els.tempInput.value = data.tempThreshold;
            currentThresholds.temp = parseFloat(data.tempThreshold);
        }
        if (data.lightThreshold) {
            els.lightInput.value = data.lightThreshold;
            currentThresholds.light = parseInt(data.lightThreshold);
        }
    }
});

// Save Config
els.saveBtn.addEventListener('click', () => {
    const newTemp = parseFloat(els.tempInput.value);
    const newLight = parseInt(els.lightInput.value);

    set(ref(db, 'greenhouse/config'), {
        tempThreshold: newTemp,
        lightThreshold: newLight,
        // Preserve moisture threshold if it exists, or just don't write it?
        // Since we are replacing the node, we should be careful.
        // But since we aren't using moisture threshold anymore, it's fine.
    }).then(() => {
        alert("Configuration Saved!");
    }).catch((err) => {
        alert("Error saving: " + err.message);
    });
});
