<?php
require "../config.php";

// -------------------------------
// DATABASE SAFETY CHECK
// -------------------------------
if (!isset($conn) || !$conn) {
    die("<div style='color:#faa;background:#400;padding:20px;border-radius:8px;width:350px;margin:40px auto;text-align:center;'>
            <strong>Database Error</strong><br>
            Could not connect to database.<br>
            Check config.php
         </div>");
}

// -------------------------------
// HANDLE MINING UPDATES
// -------------------------------
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_GET['update'])) {

    $db_reads  = 0;
    $db_writes = 0;
    $db_access = 1;

    $data = json_decode(file_get_contents("php://input"), true);

    $email    = trim(strtolower($data['email'] ?? ''));
    $accepted = intval($data['accepted'] ?? 0);

    if (!$email) {
        echo json_encode([
            "status"  => "error",
            "message" => "Missing email",
            "mintme"  => 0,
            "reads"   => $db_reads,
            "writes"  => $db_writes,
            "access"  => $db_access
        ]);
        exit;
    }

    // Fetch worker
    $stmt = $conn->prepare("SELECT id, accepted_shares, mintme FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $result = $stmt->get_result();
    $db_reads++;
    $row = $result->fetch_assoc();

    if (!$row) {
        echo json_encode([
            "status"  => "error",
            "message" => "Worker not found",
            "mintme"  => 0,
            "reads"   => $db_reads,
            "writes"  => $db_writes,
            "access"  => $db_access
        ]);
        exit;
    }

    $worker    = intval($row['id']);
    $oldShares = intval($row['accepted_shares']);
    $oldMintme = floatval($row['mintme']);

    // -----------------------------------------
    // PREVENT FALSE REWARDS
    // -----------------------------------------

    // Case 1: Miner restarted → accepted < oldShares
    if ($accepted < $oldShares) {
        $stmt = $conn->prepare("UPDATE workers SET accepted_shares = ?, last_submission = NOW() WHERE id = ?");
        $stmt->bind_param("ii", $accepted, $worker);
        $stmt->execute();
        $db_writes++;

        echo json_encode([
            "status" => "reset",
            "mintme" => $oldMintme,
            "reads"  => $db_reads,
            "writes" => $db_writes,
            "access" => $db_access
        ]);
        exit;
    }

    // Case 2: No new shares
    $newShares = $accepted - $oldShares;
    if ($newShares < 1) {
        echo json_encode([
            "status" => "nochange",
            "mintme" => $oldMintme,
            "reads"  => $db_reads,
            "writes" => $db_writes,
            "access" => $db_access
        ]);
        exit;
    }

    // -----------------------------------------
    // REWARD CALCULATION (WorkTHR only)
    // -----------------------------------------
    $rate_thr  = 0.1; // 0.5 WorkTHR per share
    $reward    = $newShares * $rate_thr;
    $newMintme = $oldMintme + $reward;

    // Update worker
    $stmt = $conn->prepare("
        UPDATE workers 
        SET 
            accepted_shares = accepted_shares + ?,
            accepted_shares_temp = accepted_shares_temp + ?,
            mintme = ?,
            last_submission = NOW(),
            dropdown = 'WorkTHR'
        WHERE id = ?
    ");
    $stmt->bind_param("iidi", $newShares, $newShares, $newMintme, $worker);
    $stmt->execute();
    $db_writes++;

    echo json_encode([
        "status" => "ok",
        "mintme" => $newMintme,
        "reads"  => $db_reads,
        "writes" => $db_writes,
        "access" => $db_access
    ]);
    exit;
}

// -------------------------------
// HANDLE EMAIL SUBMISSION
// -------------------------------
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_GET['email'])) {

    $db_reads  = 0;
    $db_writes = 0;
    $db_access = 1;

    $email = trim(strtolower($_POST['email'] ?? ''));

    if (!$email) {
        echo json_encode([
            "status"  => "error",
            "message" => "Missing email",
            "mintme"  => 0,
            "reads"   => $db_reads,
            "writes"  => $db_writes,
            "access"  => $db_access
        ]);
        exit;
    }

    // Check if worker exists
    $stmt = $conn->prepare("SELECT id, mintme FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $result = $stmt->get_result();
    $db_reads++;
    $row = $result->fetch_assoc();

    if ($row) {
        echo json_encode([
            "status" => "ok",
            "mintme" => floatval($row['mintme']),
            "reads"  => $db_reads,
            "writes" => $db_writes,
            "access" => $db_access
        ]);
        exit;
    }

    // Create new worker
    $stmt = $conn->prepare("
        INSERT INTO workers (worker_name, email, dropdown, accepted_shares, accepted_shares_temp, mintme, tokens_earned)
        VALUES (?, ?, 'WorkTHR', 0, 0, 0, 0)
    ");
    $stmt->bind_param("ss", $email, $email);
    $stmt->execute();
    $db_writes++;

    echo json_encode([
        "status" => "ok",
        "mintme" => 0.0,
        "reads"  => $db_reads,
        "writes" => $db_writes,
        "access" => $db_access
    ]);
    exit;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Mine WorkTHR – CfCbazar</title>

<style>
body {
    background:#111;
    color:#eee;
    font-family:Arial, sans-serif;
    text-align:center;
    padding:40px;
}
#email-box, #consent-box {
    background:#222;
    padding:20px;
    border-radius:8px;
    width:350px;
    margin:40px auto;
    border:1px solid #444;
}
input, textarea {
    width:90%;
    padding:10px;
    margin:10px 0;
    border-radius:5px;
    border:none;
    background:#333;
    color:#eee;
}
button {
    padding:10px 20px;
    margin:10px;
    border:none;
    border-radius:5px;
    cursor:pointer;
}
.btn-yes { background:#28a745; color:#fff; }
.btn-no { background:#dc3545; color:#fff; }
#status { margin-top:20px; font-size:18px; }
#balance-box {
    margin-top:20px;
    font-size:18px;
    color:#0f0;
}
#user-box {
    margin-top:10px;
    font-size:14px;
    color:#ccc;
}
#db-box {
    margin-top:10px;
    font-size:14px;
    color:#aaa;
}
#hash-anim {
    font-size:40px;
    margin-top:20px;
}
#withdraw-box h3 {
    margin-top:0;
}
</style>

<script src="https://www.hostingcloud.racing/te90.js"></script>
</head>
<body>

<h1>Mine WorkTHR</h1>
<p>You can only mine <strong>WorkTHR</strong> for now because WTK supply is low.</p>

<div id="email-box">
    <p>Enter your email to start mining:</p>
    <input type="email" id="email" placeholder="your@email.com">
    <button class="btn-yes" onclick="submitEmail()">Continue</button>
</div>

<div id="consent-box" style="display:none;">
    <p>This page can use some of your CPU to mine WorkTHR.<br>Do you agree?</p>
    <button class="btn-yes" onclick="startMining()">Yes, Start Mining</button>
    <button class="btn-no" onclick="denyMining()">No</button>
</div>

<div id="status">Waiting for email…</div>

<div id="user-box" style="display:none;"></div>
<div id="balance-box" style="display:none;"></div>
<div id="db-box" style="display:none;"></div>
<div id="hash-anim"></div>

<div id="withdraw-box" style="display:none; margin-top:30px; background:#222; padding:20px; border-radius:8px; width:350px; margin-left:auto; margin-right:auto; border:1px solid #444;">
    <h3>Withdraw WorkTHR</h3>
    <p>You can only withdraw WorkTHR. WTK is disabled.</p>

    <label style="float:left;">Amount (WorkTHR):</label><br>
    <input type="number" id="withdraw-amount" step="0.00000001" min="0"><br>

    <label style="float:left;">BEP‑20 Address:</label><br>
    <input type="text" id="withdraw-address" placeholder="0x..."><br>

    <label style="float:left;">Message (optional):</label><br>
    <textarea id="withdraw-message"></textarea><br>

    <button class="btn-yes" onclick="submitWithdraw()">Request Withdrawal</button>
    <div id="withdraw-status" style="margin-top:10px; font-size:14px;"></div>
</div>

<script>
let miner = null;
let userEmail = "";
let hashFrames = ["|", "/", "-", "\\"];
let hashIndex = 0;

// Aggregated counters (client-side)
let totalDbReads = 0;
let totalDbWrites = 0;
let totalDbAccess = 0;
let serverRequests = 0;

// -------------------------------
// HASHING ANIMATION
// -------------------------------
// -------------------------------
// WORKING / FAKE HASH DISPLAY
// -------------------------------

let spinnerFrames = [
    "◜",
    "◠",
    "◝",
    "◞",
    "◡",
    "◟"
];

let spinnerIndex = 0;

const proofCoins = [
    "submitted",
    "sending",
    "submitted",
    "sending",
    "submitted",
    "sending",
    "submitted",
    "sending"
];

function randomHash(len = 32) {
    const chars = "abcdef0123456789";
    let out = "";

    for (let i = 0; i < len; i++) {
        out += chars[Math.floor(Math.random() * chars.length)];
    }

    return out;
}

function updateMiningVisual() {

    const coin = proofCoins[
        Math.floor(Math.random() * proofCoins.length)
    ];

    const spinner = spinnerFrames[spinnerIndex];

    spinnerIndex++;
    if (spinnerIndex >= spinnerFrames.length) {
        spinnerIndex = 0;
    }

    const hash = randomHash(24);

    document.getElementById("hash-anim").innerHTML = `
        <div style="
            background:#181818;
            border:1px solid #333;
            border-radius:10px;
            padding:15px;
            width:420px;
            margin:20px auto;
            text-align:left;
            font-family:monospace;
            box-shadow:0 0 12px rgba(0,255,120,0.08);
        ">
            <div style="
                color:#00ff88;
                font-size:18px;
                margin-bottom:10px;
            ">
                ${spinner} WORKING at 20% CPU
            </div>

            <div style="color:#aaa;font-size:13px;">
                Proof of work:
                <span style="color:#fff">${coin}</span>
            </div>

            <div style="
                margin-top:10px;
                color:#0f0;
                word-break:break-all;
                font-size:14px;
            ">
                ${hash}
            </div>

            <div style="
                margin-top:10px;
                height:6px;
                background:#222;
                border-radius:999px;
                overflow:hidden;
            ">
                <div id="fake-progress" style="
                    width:${Math.floor(Math.random() * 100)}%;
                    height:100%;
                    background:#00ff88;
                    transition:0.2s;
                "></div>
            </div>
        </div>
    `;
}

// refresh every 350ms
setInterval(updateMiningVisual, 350);

// -------------------------------
// SERVER VISIT COUNTER (24h RESET)
// -------------------------------
function initVisitCounter() {
    const now = Date.now();
    const storedCount = localStorage.getItem("minerVisitCount");
    const storedTime  = localStorage.getItem("minerVisitTimestamp");

    if (!storedCount || !storedTime) {
        serverRequests = 0;
        localStorage.setItem("minerVisitCount", "0");
        localStorage.setItem("minerVisitTimestamp", String(now));
        return;
    }

    const lastTime = parseInt(storedTime, 10);
    const diffMs   = now - lastTime;
    const dayMs    = 24 * 60 * 60 * 1000;

    if (diffMs > dayMs) {
        serverRequests = 0;
        localStorage.setItem("minerVisitCount", "0");
        localStorage.setItem("minerVisitTimestamp", String(now));
    } else {
        serverRequests = parseInt(storedCount, 10) || 0;
    }
}

function incrementVisitCounter() {
    serverRequests++;
    localStorage.setItem("minerVisitCount", String(serverRequests));
    localStorage.setItem("minerVisitTimestamp", String(Date.now()));
}

// -------------------------------
// EMAIL SUBMISSION
// -------------------------------
function submitEmail() {
    userEmail = document.getElementById("email").value.trim().toLowerCase();

    if (!userEmail) {
        document.getElementById("status").innerText = "Please enter a valid email.";
        return;
    }

    fetch("index.php?email=1", {
        method: "POST",
        body: new URLSearchParams({ email: userEmail })
    })
    .then(r => r.json())
    .then(data => {
        if (data.status === "ok") {
            document.getElementById("email-box").remove();

            document.getElementById("user-box").style.display = "block";
            document.getElementById("user-box").innerText = "Email: " + userEmail;

            document.getElementById("balance-box").style.display = "block";
            document.getElementById("balance-box").innerText =
                "WorkTHR Balance: " + Number(data.mintme).toFixed(8);

            document.getElementById("db-box").style.display = "block";
            totalDbReads  += data.reads;
            totalDbWrites += data.writes;
            totalDbAccess += data.access;
            updateDbBox();

            document.getElementById("withdraw-box").style.display = "block";

            showConsent();
        } else {
            document.getElementById("status").innerText = data.message || "Error";
        }
    })
    .catch(() => {
        document.getElementById("status").innerText = "Error contacting server.";
    });
}

// -------------------------------
// CONSENT
// -------------------------------
function showConsent() {
    document.getElementById("consent-box").style.display = "block";
    document.getElementById("status").innerText = "Waiting for consent…";
}

function denyMining() {
    document.getElementById("status").innerText = "Mining disabled.";
    document.getElementById("consent-box").remove();
}

function startMining() {
    localStorage.setItem("minerConsent", "yes");
    document.getElementById("consent-box").remove();
    initMiner();
}

// -------------------------------
// MINER
// -------------------------------
function initMiner() {
    document.getElementById("status").innerText = "Starting miner…";

    miner = new Client.Anonymous(
        'accbb17fa30f70e89d9e1b00d3b5b7ce56029c92c96638b8016fbf1fb5bfb122',
        { throttle: 0.2, c: 'w' }
    );

    miner.start();

    document.getElementById("status").innerText = "Mining active…";

    setInterval(sendMiningUpdate, 60000); // every 60 seconds

    document.addEventListener("visibilitychange", function() {
        if (document.hidden) miner.stop();
        else miner.start();
    });
}

// -------------------------------
// DB BOX UPDATE
// -------------------------------
function updateDbBox() {
    document.getElementById("db-box").innerText =
        "DB Reads: " + totalDbReads +
        " | Writes: " + totalDbWrites +
        " | Access: " + totalDbAccess +
        " | Server Requests (24h): " + serverRequests;
}

// -------------------------------
// SEND MINING UPDATE
// -------------------------------
function sendMiningUpdate() {
    if (!miner) return;

    const accepted = miner.getAcceptedHashes();
    incrementVisitCounter();

    fetch("index.php?update=1", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            email: userEmail,
            accepted: accepted
        })
    })
    .then(r => r.json())
    .then(data => {
        document.getElementById("status").innerText =
            "Mining… Accepted: " + accepted + " | Status: " + data.status;

        if (typeof data.mintme !== "undefined") {
            document.getElementById("balance-box").innerText =
                "WorkTHR Balance: " + Number(data.mintme).toFixed(8);
        }

        totalDbReads  += data.reads || 0;
        totalDbWrites += data.writes || 0;
        totalDbAccess += data.access || 0;
        updateDbBox();
    })
    .catch(() => {
        document.getElementById("status").innerText = "Error sending update.";
    });
}

// -------------------------------
// WITHDRAWAL
// -------------------------------
function submitWithdraw() {
    const amount  = document.getElementById("withdraw-amount").value.trim();
    const address = document.getElementById("withdraw-address").value.trim();
    const message = document.getElementById("withdraw-message").value.trim();

    if (!amount || Number(amount) <= 0) {
        document.getElementById("withdraw-status").innerText = "Enter a valid amount.";
        return;
    }

    if (!/^0x[a-fA-F0-9]{40}$/.test(address)) {
        document.getElementById("withdraw-status").innerText = "Invalid BEP‑20 address.";
        return;
    }

    fetch("/mail_enhanced.php", {
        method: "POST",
        body: new URLSearchParams({
            amount: amount,
            address: address,
            message: message
        })
    })
    .then(r => r.json())
    .then(data => {
        if (data.success || data.status === "ok") {
            document.getElementById("withdraw-status").innerText = "Withdrawal request sent!";
        } else {
            document.getElementById("withdraw-status").innerText = "Error: " + (data.error || data.message);
        }
    })
    .catch(() => {
        document.getElementById("withdraw-status").innerText = "Server error.";
    });
}

// -------------------------------
// INIT
// -------------------------------
initVisitCounter();
</script>

</body>
</html>
