// Set up the temperature chart
const tempCtx = document.getElementById('temperatureChart').getContext('2d');
const tempChart = new Chart(tempCtx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Temperature (°C)',
            data: [],
            borderColor: 'rgba(255, 99, 132, 1)',
            backgroundColor: 'rgba(255, 99, 132, 0.2)',
            borderWidth: 2,
            fill: true
        }]
    },
    options: {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'Temperature (°C)' } }
        }
    }
});

// Set up the air quality chart
const mqCtx = document.getElementById('mq135Chart').getContext('2d');
const mqChart = new Chart(mqCtx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Air Quality (ppm)',
            data: [],
            borderColor: 'rgba(75, 192, 192, 1)',
            backgroundColor: 'rgba(75, 192, 192, 0.2)',
            borderWidth: 2,
            fill: true
        }]
    },
    options: {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'Air Quality (ppm)' } }
        }
    }
});

// Set up the sound level chart
const soundCtx = document.getElementById('soundChart').getContext('2d');
const soundChart = new Chart(soundCtx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Sound Level (dB)',
            data: [],
            borderColor: 'rgba(153, 102, 255, 1)',
            backgroundColor: 'rgba(153, 102, 255, 0.2)',
            borderWidth: 2,
            fill: true
        }]
    },
    options: {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'Sound Level (dB)' } }
        }
    }
});

// Server details (replace with the IP printed from the Arduino Serial Monitor)
const host = "";  // Replace with your Arduino's IP
const port = 5000;             // Port number for the backend

// Connect to Arduino via Web Serial API and fetch data
async function fetchArduinoData() {
    try {
        // Connect to the serial port (if using Web Serial API)
        const port = await navigator.serial.requestPort();
        await port.open({ baudRate: 9600 });

        // Set up the text decoder for reading serial data
        const textDecoder = new TextDecoderStream();
        const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
        const reader = textDecoder.readable.getReader();

        let buffer = ""; // Buffer to store incomplete data

        // Read incoming serial data
        while (true) {
            const { value, done } = await reader.read();
            if (done) {
                reader.releaseLock();
                break;
            }
            if (value) {
                buffer += value; // Append new data to the buffer

                // Split the buffer by newlines
                const lines = buffer.split("\n");
                buffer = lines.pop();  // Keep the incomplete line in buffer

                // Process each complete line of data
                for (const line of lines) {
                    if (line.trim()) {
                        processArduinoData(line.trim());
                    }
                }
            }
        }
    } catch (error) {
        console.error("Error connecting to Arduino:", error);
    }
}

// Process the data from Arduino and update the charts
async function processArduinoData(data) {
    try {
        // Parse the JSON data received from Arduino
        const parsedData = JSON.parse(data);

        // Get the current time for the x-axis labels
        const now = new Date().toLocaleTimeString();

        // Update the Temperature Chart
        tempChart.data.labels.push(now);
        tempChart.data.datasets[0].data.push(parsedData.temperature);
        tempChart.update();

        // Update the Air Quality Chart
        mqChart.data.labels.push(now);
        mqChart.data.datasets[0].data.push(parsedData.airQuality);
        mqChart.update();

        // Update the Sound Level Chart
        soundChart.data.labels.push(now);
        soundChart.data.datasets[0].data.push(parsedData.soundLevel);
        soundChart.update();

        // Update stats on the page
        document.getElementById('avg-temp').textContent = `Temperature: ${parsedData.temperature.toFixed(1)} °C`;
        document.getElementById('avg-air').textContent = `Air Quality: ${parsedData.airQuality} ppm`;
        document.getElementById('avg-sound').textContent = `Sound Level: ${parsedData.soundLevel} dB`;

        // Display warning if thresholds are exceeded
        const warningElement = document.getElementById('warning');
        if (parsedData.temperature > 30 || parsedData.airQuality > 300 || parsedData.soundLevel > 80) {
            warningElement.style.display = 'block';
            warningElement.textContent = 'Warning: Threshold exceeded!';
        } else {
            warningElement.style.display = 'none';
        }

        // Send the sensor data to the backend (Flask server)
        const response = await fetch(`http://${host}:${port}/data`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(parsedData)  // Send the parsed data to the server
        });

        if (!response.ok) {
            console.error("Failed to send data to backend");
        }
    } catch (error) {
        console.error("Error processing Arduino data:", error.message);
    }
}

// Start fetching Arduino data
fetchArduinoData();
