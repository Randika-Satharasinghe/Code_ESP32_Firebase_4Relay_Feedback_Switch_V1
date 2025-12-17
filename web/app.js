import { initializeApp } from "https://www.gstatic.com/firebasejs/9.22.0/firebase-app.js";
import { getDatabase, ref, onValue, set, query, limitToLast, onChildAdded } from "https://www.gstatic.com/firebasejs/9.22.0/firebase-database.js";

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
    loc: document.getElementById('locValue'),
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
            { label: 'Moisture', borderColor: '#ff5252', data: [] },
            { label: 'Light', borderColor: '#2979ff', data: [] },
            // Moisture/Light hidden on chart to keep it clean, can be added if requested
        ]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
            x: {
                grid: { color: 'rgba(255,255,255,0.1)' },
                ticks: { color: '#ccc' }
            },
            y: {
                grid: { color: 'rgba(255,255,255,0.1)' },
                ticks: { color: '#ccc' }
            }
        },
        plugins: {
            legend: { labels: { color: 'white' } }
        }
    }
});

// Update Chart with new data point
function addDataToChart(timestamp, moisture, light) {
    const date = new Date(timestamp);
    const timeLabel = date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    if (sensorChart.data.labels.length > 50) {
        sensorChart.data.labels.shift();
        sensorChart.data.datasets.forEach(bs => bs.data.shift());
    }

    sensorChart.data.labels.push(timeLabel);
    sensorChart.data.datasets[0].data.push(moisture);
    sensorChart.data.datasets[1].data.push(light);
    sensorChart.update();
}

// Listen to Real-time Status (Cards & Actuators)
const statusRef = ref(db, 'greenhouse/status');
onValue(statusRef, (snapshot) => {
    try {
        const data = snapshot.val();
        console.log("Status Data:", data);
        if (data) {
            statusBadge.textContent = "Connected";
            statusBadge.style.color = "#00e676";

            // Fix for 0 values
            els.temp.textContent = (data.moisturePercent !== undefined) ? data.moisturePercent : "--";
            els.hum.textContent = (data.moistureRaw !== undefined) ? data.moistureRaw : "--";
            els.moist.textContent = (data.moisturePercent !== undefined && data.moisturePercent !== null) ? data.moisturePercent : "--";
            els.light.textContent = (data.light !== undefined && data.light !== null) ? data.light : "--";

            // Location
            if (data.lat && data.lng) {
                els.loc.innerHTML = `Lat: ${data.lat}<br>Lng: ${data.lng}`;
            } else {
                els.loc.textContent = "No GPS Data";
            }

            // Actuators disabled, but keeping status display
            els.fanStatus.textContent = "DISABLED";
            els.fanStatus.className = "status-pill off";
            els.lightStatus.textContent = "DISABLED";
            els.lightStatus.className = "status-pill off";
        }
    } catch (e) { console.error(e); }
});

// Listen to History for Chart (Last 50 points)
const historyRef = query(ref(db, 'greenhouse/history'), limitToLast(50));
onChildAdded(historyRef, (snapshot) => {
    const data = snapshot.val();
    if (data && data.ts && data.moisturePercent !== undefined && data.light !== undefined) {
        addDataToChart(data.ts, data.moisturePercent, data.light);
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
    }).then(() => {
        alert("Configuration Saved!");
    }).catch((err) => {
        alert("Error saving: " + err.message);
    });
});
