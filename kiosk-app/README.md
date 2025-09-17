# TotoBin Kiosk (ตู้โตโต้บิน)

A lightweight, touch-friendly kiosk application for ordering drinks with PromptPay QR code payment integration.

## Features

✨ **Complete Kiosk Flow**
- Landing page with auto-rotating advertisements
- Menu selection with customizable options (size, sweetness, toppings)
- QR code payment with 60-second timer
- Real-time order tracking and brewing simulation
- Receipt and completion confirmation

🎨 **Modern UX/UI**
- Touch-optimized interface with large buttons (56px+ hit areas)
- Mint-green theme with smooth animations
- Responsive design for 1080p landscape displays
- Accessibility-compliant contrast and focus states

💳 **PromptPay Integration**
- Real PromptPay QR code generation using phone: `098-443-5255`
- Mock webhook for testing payment success
- 60-second payment timeout with auto-cancellation

🔒 **Single-Order Locking**
- Prevents multiple concurrent orders during brewing
- Automatic lock/unlock management
- Clear visual feedback when locked

📱 **PWA Ready**
- Offline fallback page
- Kiosk-mode manifest configuration
- Service worker for caching

## Tech Stack

- **Framework**: Next.js 14+ with App Router
- **Language**: TypeScript
- **Styling**: Tailwind CSS + shadcn/ui
- **State Management**: Zustand
- **API Calls**: TanStack Query
- **Animations**: Framer Motion
- **QR Generation**: promptpay-qr + qrcode
- **PWA**: next-pwa

## Quick Start

### Development

```bash
# Install dependencies
npm install

# Start development server
npm run dev

# Open http://localhost:3000
```

### Production Build

```bash
# Build for production
npm run build

# Start production server
npm start
```

### Kiosk Mode Setup (Chromium)

```bash
# Launch in kiosk mode
chromium-browser --kiosk --no-sandbox --disable-web-security http://localhost:3000

# Or with auto-restart script
pm2 start ecosystem.config.js
```

## API Endpoints

### Orders
- `POST /api/orders/create` - Create new order
- `GET /api/orders/status/[orderId]` - Get order status
- `POST /api/orders/lock` - Lock kiosk for single order
- `DELETE /api/orders/lock` - Unlock kiosk

### Payments
- `POST /api/payments/create` - Generate PromptPay QR
- `GET /api/payments/status/[orderId]` - Check payment status
- `POST /api/payments/mock-webhook` - Mock payment success (testing)

### Device Control (Stubs)
- `POST /api/device/start` - Start brewing process
- `GET /api/device/heartbeat` - Device health check

## Configuration

Create `.env.local`:

```env
KIOSK_TITLE="TotoBin Kiosk"
KIOSK_LOCK_TIMEOUT_SEC=120
PAYMENT_PROVIDER="mock"
PROMPTPAY_PHONE="098-443-5255"
```

## Testing the Flow

1. **Landing Page**: Auto-rotating ads, click to start ordering
2. **Menu Selection**: Choose drink, customize size/sweetness/toppings
3. **Checkout**: See QR code, use "🧪 จำลองการชำระเงินสำเร็จ" button to simulate payment
4. **In Progress**: Watch brewing animation and status updates
5. **Done**: View receipt and auto-return to landing

## State Flow

```
IDLE → AWAITING_PAYMENT (60s timer) → PAID → DISPENSING → DONE → IDLE
       ↓ (timeout/cancel)              ↓ (error)
       CANCELLED → IDLE                ERROR → IDLE
```

## Future Enhancements

- **ESP32 Integration**: Replace device stubs with MQTT/HTTP communication
- **Real Payment Gateway**: Replace mock with actual PromptPay provider
- **Database**: Replace in-memory stores with SQLite/PostgreSQL
- **Analytics**: Order tracking and sales reporting
- **Multi-language**: Support English and other languages

## Deployment

### Hardware Requirements
- Odroid C4 or similar ARM64 device
- Touch screen display (1080p recommended)
- Stable internet connection
- Optional: Thermal printer for receipts

### Software Setup
```bash
# Install Node.js 18+
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Install PM2 for process management
npm install -g pm2

# Clone and setup project
git clone <repository>
cd kiosk-app
npm install
npm run build

# Start with PM2
pm2 start npm --name "kiosk" -- start
pm2 startup
pm2 save
```

### Kiosk Mode Setup
```bash
# Install Chromium
sudo apt-get install chromium-browser

# Create startup script
sudo nano /etc/systemd/system/kiosk.service

# Configure auto-login and kiosk launch
# See deployment docs for full configuration
```

## License

MIT License - see LICENSE file for details.

## Support

For questions or issues, please contact the development team.

---

**Made with � by the TotoBin Team**
