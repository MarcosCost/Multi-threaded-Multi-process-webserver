// --------------------------------------------------------------------------
// 1. Variable Declarations
// --------------------------------------------------------------------------

// A global variable to track the count
let currentCount = 0;

// Get references to the HTML elements we need to interact with
const counterDisplay = document.getElementById('count-display');
const incrementButton = document.getElementById('increment-btn');
const resetButton = document.getElementById('reset-btn');

// --------------------------------------------------------------------------
// 2. Core Logic (Functions)
// --------------------------------------------------------------------------

/**
 * Updates the counter and displays the new value on the page.
 */
function updateCounter() {
    // Increment the count
    currentCount++;
    
    // Update the text content of the display element
    if (counterDisplay) {
        counterDisplay.textContent = currentCount;
    }
}

/**
 * Resets the counter to zero.
 */
function resetCounter() {
    currentCount = 0;
    
    // Update the display
    if (counterDisplay) {
        counterDisplay.textContent = currentCount;
    }
    
    console.log('Counter has been reset.');
}

// --------------------------------------------------------------------------
// 3. Event Listeners (DOM Interaction)
// --------------------------------------------------------------------------

// Check if the button elements exist before attaching listeners
if (incrementButton) {
    // Attach the updateCounter function to the button's click event
    incrementButton.addEventListener('click', updateCounter);
    console.log('Increment button listener attached.');
}

if (resetButton) {
    // Attach the resetCounter function to the button's click event
    resetButton.addEventListener('click', resetCounter);
    console.log('Reset button listener attached.');
}

// Log initial state to the console
console.log(`Script loaded. Initial count: ${currentCount}`);