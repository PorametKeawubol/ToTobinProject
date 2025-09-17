import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  experimental: {
    optimizePackageImports: ["lucide-react", "@radix-ui/react-icons"],
  },
  images: {
    domains: ["localhost"],
  },
};

// Use dynamic import for next-pwa to avoid type issues
let withPWA: any;

if (process.env.NODE_ENV === "production") {
  withPWA = require("next-pwa")({
    dest: "public",
    disable: false,
    register: true,
    skipWaiting: true,
    runtimeCaching: [
      {
        urlPattern: /^https?.*/,
        handler: "NetworkFirst",
        options: {
          cacheName: "offlineCache",
          expiration: {
            maxEntries: 200,
          },
        },
      },
    ],
    fallbacks: {
      document: "/offline.html",
    },
  });

  module.exports = withPWA(nextConfig);
} else {
  module.exports = nextConfig;
}
