<?php
require 'config.php';
header('Content-Type: application/json');

function is_valid_email($email) {
    return filter_var($email, FILTER_VALIDATE_EMAIL);
}

function is_valid_mac($mac) {
    return preg_match('/^([0-9A-F]{2}:){5}[0-9A-F]{2}$/i', $mac);
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'POST') {
    $email      = $_POST['email'] ?? '';
    $tokens     = $_POST['tokens'] ?? null;
    $mac        = $_POST['mac_address'] ?? '';
    $token_type = $_POST['token_type'] ?? 'WorkToken'; // default to WorkToken

    if (!is_valid_email($email) || !is_numeric($tokens)) {
        http_response_code(400);
        echo json_encode(['success' => false, 'error' => 'Invalid email or token value']);
        exit;
    }

    $tokens = floatval($tokens);

    // Handle sync-trigger reset from testapi.php
    if ($mac === 'sync-trigger' && $tokens === 0.12345) {
        $stmt = $conn->prepare("UPDATE workers SET tokens_earned = 0, mintme = 0 WHERE email = ?");
        $stmt->bind_param("s", $email);
        $ok = $stmt->execute();
        $stmt->close();
        echo json_encode([
            'success' => $ok,
            'action'  => 'reset',
            'reason'  => 'Sync-trigger reset',
            'email'   => $email
        ]);
        exit;
    }

    // Reject invalid mining amounts
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

    // Enforce cooldown
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

        $stmt = $conn->prepare("
            INSERT INTO devices (email, mac_address, last_mine_time, active)
            VALUES (?, ?, NOW(), 1)
            ON DUPLICATE KEY UPDATE last_mine_time = NOW(), active = 1
        ");
        $stmt->bind_param("ss", $email, $mac);
        $stmt->execute();
        $stmt->close();
    }

    // Determine field
    $field = ($token_type === 'WorkTHR') ? 'mintme' : 'tokens_earned';

    // Check cfcbazar@gmail.com balance
    $stmt = $conn->prepare("SELECT $field FROM workers WHERE email = 'cfcbazar@gmail.com'");
    $stmt->execute();
    $stmt->bind_result($platform_balance);
    $stmt->fetch();
    $stmt->close();

    $platform_balance = $platform_balance === null ? 0.0 : floatval($platform_balance);
    if ($platform_balance < $tokens) {
        http_response_code(403);
        echo json_encode([
            'success' => false,
            'error' => 'Mining disabled: platform has no tokens left. Try again later.',
            'token_type' => $token_type,
            'platform_balance' => $platform_balance
        ]);
        exit;
    }

    // Begin transaction
    $conn->begin_transaction();

    // Deduct from platform
    $stmt = $conn->prepare("UPDATE workers SET $field = $field - ? WHERE email = 'cfcbazar@gmail.com'");
    $stmt->bind_param("d", $tokens);
    $stmt->execute();
    $stmt->close();

    // Credit user
    $stmt = $conn->prepare("UPDATE workers SET $field = $field + ?, last_mine_time = NOW() WHERE email = ?");
    $stmt->bind_param("ds", $tokens, $email);
    $stmt->execute();
    $stmt->close();

    $conn->commit();

    echo json_encode([
        'success'      => true,
        'email'        => $email,
        'token_type'   => $token_type,
        'tokens_delta' => $tokens,
        'platform_deducted' => $tokens,
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

    $stmt = $conn->prepare("SELECT tokens_earned, mintme FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $stmt->bind_result($tokens, $mintme);
    $stmt->fetch();
    $stmt->close();

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
        'mintme'        => (float)$mintme,
        'devices'       => $devices
    ]);
    exit;
}

http_response_code(405);
echo json_encode(['success' => false, 'error' => 'Method not allowed']); 
