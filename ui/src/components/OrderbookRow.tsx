import { useMemo } from "react";

interface OrderbookRowProps {
  price: number;
  quantity: number;
  maxQuantity: number;
  type: "bid" | "ask";
}

export const OrderbookRow = ({ price, quantity, maxQuantity, type }: OrderbookRowProps) => {
  const depthPercentage = useMemo(() => {
    return (quantity / maxQuantity) * 100;
  }, [quantity, maxQuantity]);

  const total = useMemo(() => {
    return (price * quantity).toFixed(2);
  }, [price, quantity]);

  return (
    <div
      className={`relative grid grid-cols-3 gap-2 px-4 py-1.5 text-sm font-mono transition-colors ${
        type === "bid" ? "hover:bg-bid/10" : "hover:bg-ask/10"
      }`}
    >
      {/* Depth bar background */}
      <div
        className={`absolute inset-y-0 ${type === "bid" ? "right-0 bg-bid-depth" : "left-0 bg-ask-depth"} transition-all duration-300`}
        style={{ width: `${depthPercentage}%` }}
      />
      
      {/* Content */}
      <div className={`relative z-10 ${type === "bid" ? "text-bid" : "text-ask"} font-semibold`}>
        {price.toFixed(5)}
      </div>
      <div className="relative z-10 text-right text-foreground/80">
        {quantity.toFixed(4)}
      </div>
      <div className="relative z-10 text-right text-muted-foreground">
        {total}
      </div>
    </div>
  );
};
