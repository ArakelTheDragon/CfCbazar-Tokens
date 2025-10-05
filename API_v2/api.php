<?php
// api hosted at cfc-api.atwebpages.com/api.php
require 'config.php'; // server file containing database connection and login
header('Content-Type: application/json');

function is_valid_email($email) {
    return filter_var($email, FILTER_VALIDATE_EMAIL);
}

function is_valid_mac($mac) {
    return preg_match('/^([0-9A-F]{2}:){5}[0-9A-F]{2}$/i', $mac);
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'POST') {
    $email  = $_POST['email'] ?? '';
    $tokens = $_POST['tokens'] ?? null;
    $mac    = $_POST['mac_address'] ?? '';

    if (!is_valid_email($email) || !is_numeric($tokens)) {
        http_response_code(400);
        echo json_encode(['success' => false, 'error' => 'Invalid email or token value']);
        exit;
    }

    $tokens = floatval($tokens);

    // Reset tokens if value is not exactly 0.00001
    if ($tokens !== 0.00001) {
        $stmt = $conn->prepare("UPDATE workers SET tokens_earned = 0 WHERE email = ?");
        $stmt->bind_param("s", $email);
        $ok = $stmt->execute();
        $stmt->close();

        echo json_encode([
            'success' => $ok,
            'action'  => 'reset',
            'reason'  => 'Invalid token value',
            'email'   => $email
        ]);
        exit;
    }

    // Enforce cooldown using devices table
    if (is_valid_mac($mac)) {
        $stmt = $conn->prepare("SELECT last_mine_time FROM devices WHERE email = ? AND mac_address = ?");
        $stmt->bind_param("ss", $email, $mac);
        $stmt->execute();
        $stmt->bind_result($last_mine);
        $stmt->fetch();
        $stmt->close();

        if ($last_mine && (time() - strtotime($last_mine)) < 5) {
            http_response_code(429);
            echo json_encode(['success' => false, 'error' => 'Rate limit: wait 5 seconds']);
            exit;
        }

        // Update or insert device record
        $stmt = $conn->prepare("
            INSERT INTO devices (email, mac_address, last_mine_time, active)
            VALUES (?, ?, NOW(), 1)
            ON DUPLICATE KEY UPDATE last_mine_time = NOW(), active = 1
        ");
        $stmt->bind_param("ss", $email, $mac);
        $stmt->execute();
        $stmt->close();
    }

    // Update tokens_earned and last_mine_time in workers
    $stmt = $conn->prepare("
        UPDATE workers
           SET tokens_earned = tokens_earned + ?,
               last_mine_time = NOW()
         WHERE email = ?
    ");
    $stmt->bind_param("ds", $tokens, $email);
    $ok = $stmt->execute();
    $stmt->close();

    echo json_encode([
        'success'      => $ok,
        'email'        => $email,
        'tokens_delta' => $tokens,
        'timestamp'    => date('Y-m-d H:i:s')
    ]);
    exit;
}

if ($method === 'GET') {
    $email = $_GET['email'] ?? '';
    if (!is_valid_email($email)) {
        http_response_code(400);
        echo json_encode(['success' => false, 'error' => 'Invalid email']);
        exit;
    }

    // Fetch tokens_earned
    $stmt = $conn->prepare("SELECT tokens_earned FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $stmt->bind_result($tokens);
    $stmt->fetch();
    $stmt->close();

    // Fetch devices
    $stmt = $conn->prepare("SELECT mac_address, last_mine_time FROM devices WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $result = $stmt->get_result();
    $devices = [];
    while ($row = $result->fetch_assoc()) {
        $devices[] = $row;
    }
    $stmt->close();

    echo json_encode([
        'success'       => true,
        'email'         => $email,
        'tokens_earned' => (float)$tokens,
        'devices'       => $devices
    ]);
    exit;
}

http_response_code(405);
echo json_encode(['success' => false, 'error' => 'Method not allowed']); 
