<?php
// ----- API.php (cfc-api.atwebpages.com/api.php) -----
require 'config.php';
header('Content-Type: application/json');

// Validate email format
function is_valid_email(string $email): bool {
    return filter_var($email, FILTER_VALIDATE_EMAIL) !== false;
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'POST') {
    $email  = $_POST['email'] ?? '';
    $tokens = $_POST['tokens'] ?? null;

    if (!is_valid_email($email) || !is_numeric($tokens)) {
        http_response_code(400);
        echo json_encode(['success' => false, 'error' => 'Invalid email or token value']);
        exit;
    }

    $tokens = floatval($tokens);

    // If tokens != 0.00001, reset tokens_earned
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

    // Enforce 5 sec rule using last_mine_time
    $stmt = $conn->prepare("SELECT last_mine_time FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $stmt->bind_result($last_mine_time);
    $stmt->fetch();
    $stmt->close();

    $now = time();
    if ($last_mine_time && ($now - strtotime($last_mine_time)) < 5) {
        http_response_code(429);
        echo json_encode(['success' => false, 'error' => 'Rate limit: wait 5 seconds']);
        exit;
    }

    // Update tokens and last_mine_time
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

    $stmt = $conn->prepare("SELECT tokens_earned FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $stmt->bind_result($tokens);
    if ($stmt->fetch()) {
        echo json_encode([
            'success'       => true,
            'email'         => $email,
            'tokens_earned' => (float)$tokens
        ]);
    } else {
        http_response_code(404);
        echo json_encode(['success' => false, 'error' => 'Worker not found']);
    }
    $stmt->close();
    exit;
}

http_response_code(405);
echo json_encode(['success' => false, 'error' => 'Method not allowed']);
