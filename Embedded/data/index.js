let host = window.location.host;
let rootPath = `http://${host}`;
let gateway = `ws://${host}/ws`;
let websocket;

let streamIntervelID = null;

let imgStream = document.getElementById("stream");
let streamCtn = document.getElementById("stream-container");
let btnStream = document.getElementById("toggle-stream");


btnStream.addEventListener("click", (e) => {
    if (!websocket || websocket.readyState == WebSocket.CLOSED) {
        startStream();
    } else {
        if (websocket.readyState == WebSocket.OPEN) {
            stopStream();
        }
    }
});

function startStream() {
    initWebSocket();

    streamIntervelID = setInterval(() => {
        websocket.send('GI');
    }, 100);


    btnStream.textContent = "Stop Stream";
    streamCtn.classList.remove("hidden");
}

function stopStream() {
    websocket.close();
    clearInterval(streamIntervelID);

    btnStream.textContent = "Start Stream";
    streamCtn.classList.add("hidden");
}

document.getElementById("close-stream").addEventListener("click", (e) => {
    if (websocket.readyState == WebSocket.OPEN) {
        stopStream();
    }
});


function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);

    websocket.onopen = (e) => {
        console.log('Connection opened');
    };

    websocket.onclose = (e) => {
        console.log('Connection closed');
    };

    websocket.onmessage = onMessageWebSocket;
}

function onMessageWebSocket(event) {
    console.log("Receive data");
    console.log(event.data);
    console.log(typeof event.data);

    if (typeof event.data === 'object') {
        let blob = event.data;
        const imageUrl = URL.createObjectURL(blob, { type: 'image/jpeg' });
        imgStream.src = imageUrl;
    }
}


document.addEventListener("DOMContentLoaded", async (e) => {
    fillLocalWifiEle();
    fillPublicWifiEle();
});

document.getElementById("public-form").addEventListener("submit", async (e) => {
    e.preventDefault();

    let data = {
        name: document.getElementById("public-wifi-name").value,
        password: document.getElementById("public-wifi-password").value
    }

    console.log("POST /change-public-wifi");
    console.log("Body: ", data);

    let response = await fetch(rootPath + "/change-public-wifi", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify(data)
    });
    
    if (response.ok) {
        alert("Save public wifi credential success");
    } else {
        alert("Save public wifi credential failed!");
    }
});

document.getElementById("local-form").addEventListener("submit", async (e) => {
    e.preventDefault();

    let data = {
        password: document.getElementById("local-wifi-password").value,
        newPassword: document.getElementById("local-wifi-new-password").value
    }

    console.log("POST /change-local-wifi");
    console.log("Body: ", data);

    let response = await fetch(rootPath + "/change-local-wifi", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify(data)
    });
    
    if (response.ok) {
        alert("Change local wifi password suscessfully!");
    } else {
        error = await response.text();
        alert(error);
    }
});

async function fillPublicWifiEle() {
    let response = await fetch(rootPath + "/public-wifi-info", { method: "GET" });
    let json = await response.json();

    console.log("GET /wifi-info");
    console.log(json);

    document.getElementById("public-wifi-name").value = json.name;
}

async function fillLocalWifiEle() {
    let response = await fetch(rootPath + "/local-wifi-info", { method: "GET" });
    let json = await response.json();

    console.log("GET /wifi-info");
    console.log(json);

    document.getElementById("local-wifi-name").value = json.name;
}