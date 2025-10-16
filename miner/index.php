<?php
// miner/index.php
require_once __DIR__ . '/../includes/reusable.php';
require_once __DIR__ . '/../includes/reusable2.php';

// Enforce HTTPS and track page visits
enforce_https();
track_visits();

$email = $_SESSION['email'] ?? null;
$logged_in = $email !== null;

// Handle auto-claim reward and device status update
if ($_SERVER['REQUEST_METHOD'] === 'POST' && $logged_in && isset($_POST['reward_type'], $_POST['accepted'], $_POST['mac_address'], $_POST['active'])) {
    handleMinerReward(
        $email,
        $_POST['reward_type'],
        intval($_POST['accepted']),
        $_POST['mac_address'],
        intval($_POST['active'])
    );
    exit;
}

$title = "⛏️ CfCbazar Miner | Earn WorkToken & WorkTHR";
include_header();
include_menu();
?>

<main class="dashboard-container">
    <section class="hero-section">
        <h2>⛏️ Start Mining WorkToken & WorkTHR</h2>
        <p>Mine platform credits directly in your browser. Stay on this tab to earn rewards.</p>
    </section>

    <?php renderMinerInterface($logged_in); ?>
    <?php renderMinerScript(); ?>

    <?php
    if ($logged_in) {
        $stats = getWorkerStats($email);
        if (!empty($stats)) {
            echo '<section class="table-container">';
            echo '<h3>📈 Your Mining Totals</h3>';
            echo '<table class="worker-stats-table" role="grid" aria-label="Mining Totals">';
            echo '<thead><tr><th>WorkToken Earned</th><th>WorkTHR Earned</th></tr></thead>';
            echo '<tbody><tr>';
            echo '<td>' . number_format((float)($stats['tokens_earned'] ?? 0), 8) . '</td>';
            echo '<td>' . number_format((float)($stats['mintme'] ?? 0), 8) . '</td>';
            echo '</tr></tbody></table></section>';
			
			// Withdraw button section
            echo '<section class="links-grid" aria-label="Withdraw WorkTokens">';
            echo '<a href="/w.php" class="link-card" aria-label="Withdraw WorkTokens">💸 <span>Withdraw WorkTokens</span></a>';
            echo '</section>';
        }
    }
    ?>
</main>


<?php
include_footer();
?>
