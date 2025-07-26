// SPDX-License-Identifier: GPL-3.0
pragma solidity ^0.8.24;

// OpenZeppelin Upgradeable Contracts
import "@openzeppelin/contracts-upgradeable/token/ERC20/ERC20Upgradeable.sol";
import "@openzeppelin/contracts-upgradeable/access/OwnableUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/proxy/utils/UUPSUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/security/ReentrancyGuardUpgradeable.sol";

contract WorkTokenStable is
    Initializable,
    ERC20Upgradeable,
    OwnableUpgradeable,
    UUPSUpgradeable,
    ReentrancyGuardUpgradeable
{
    uint256 public marketPrice; // in wei per WTK
    uint256 public recycledWTK;

    event Bought(address indexed buyer, uint256 bnbSent, uint256 wtkReceived);
    event Sold(address indexed seller, uint256 wtkSold, uint256 bnbReturned);
    event Recycled(uint256 amount);
    event Burned(uint256 amount);
    event MarketPriceSet(uint256 newPrice);
    event OwnerFunded(uint256 bnb);
    event OwnerWithdrawn(uint256 bnb);

    /// @custom:oz-upgrades-unsafe-allow constructor
    constructor() {
        _disableInitializers();
    }

    function initialize(address initialOwner, uint256 initialPrice) public initializer {
        __ERC20_init("WorkToken", "WTK");
        __Ownable_init(initialOwner);
        __UUPSUpgradeable_init();
        __ReentrancyGuard_init();

        marketPrice = initialPrice;
    }

    function setMarketPrice(uint256 newPrice) external onlyOwner {
        require(newPrice >= 1e13, "Price too low");
        marketPrice = newPrice;
        emit MarketPriceSet(newPrice);
    }

    function buy() external payable nonReentrant {
        require(msg.value > 0, "No BNB sent");

        uint256 buyPrice = marketPrice + (marketPrice / 10); // +10%
        uint256 tokens = (msg.value * 1e18) / buyPrice;
        require(tokens >= 1e9, "Buy amount too low");

        uint256 fromRecycle = recycledWTK >= tokens ? tokens : recycledWTK;
        uint256 toMint = tokens - fromRecycle;

        if (fromRecycle > 0) {
            unchecked {
                recycledWTK -= fromRecycle;
            }
            _transfer(address(this), msg.sender, fromRecycle);
        }

        if (toMint > 0) {
            _mint(msg.sender, toMint);
        }

        emit Bought(msg.sender, msg.value, tokens);
    }

    function sell(uint256 amount) external nonReentrant {
        require(amount >= 1e9, "Sell amount too low");

        uint256 sellPrice = marketPrice - (marketPrice / 10); // -10%
        uint256 bnbOut = (amount * sellPrice) / 1e18;
        require(address(this).balance >= bnbOut, "Contract lacks BNB");

        _transfer(msg.sender, address(this), amount);
        unchecked {
            recycledWTK += amount;
        }

        (bool sent, ) = payable(msg.sender).call{value: bnbOut}("");
        require(sent, "BNB transfer failed");

        emit Sold(msg.sender, amount, bnbOut);
        emit Recycled(amount);
    }

    function burnFromRecycle(uint256 amount) external onlyOwner {
        require(recycledWTK >= amount, "Not enough recycled tokens");
        _burn(address(this), amount);
        unchecked {
            recycledWTK -= amount;
        }
        emit Burned(amount);
    }

    function fund() external payable onlyOwner {
        require(msg.value > 0, "No BNB sent");
        emit OwnerFunded(msg.value);
    }

    function withdraw(uint256 amount) external onlyOwner {
        require(address(this).balance >= amount, "Not enough BNB");
        (bool sent, ) = payable(msg.sender).call{value: amount}("");
        require(sent, "Withdraw failed");
        emit OwnerWithdrawn(amount);
    }

    function _authorizeUpgrade(address newImplementation) internal override onlyOwner {}

    receive() external payable {}
}
