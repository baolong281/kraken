import { useMemo } from "react";
import { OrderbookRow } from "./OrderbookRow";
import { Card } from "./ui/card";

interface Order {
  price_: number;
  qty_: number;
}

interface OrderbookData {
  symbol: string;
  bid: Order[];
  asks: Order[];
}

interface OrderbookDisplayProps {
  data: OrderbookData;
}

export const OrderbookDisplay = ({ data }: OrderbookDisplayProps) => {
  const spread = useMemo(() => {
    if (data.asks.length === 0 || data.bid.length === 0) return 0;
    return data.asks[0].price_ - data.bid[0].price_;
  }, [data]);

  const spreadPercentage = useMemo(() => {
    if (data.bid.length === 0) return 0;
    return (spread / data.bid[0].price_) * 100;
  }, [spread, data]);

  const maxBidQty = useMemo(() => {
    return Math.max(...data.bid.map((b) => b.qty_));
  }, [data.bid]);

  const maxAskQty = useMemo(() => {
    return Math.max(...data.asks.map((a) => a.qty_));
  }, [data.asks]);

  return (
    <Card className="overflow-hidden">
      {/* Header */}
      <div className="border-b border-border bg-card px-4 py-3">
        <div className="flex items-center justify-between">
          <h2 className="text-xl font-bold">{data.symbol}</h2>
          <div className="flex items-center gap-2 text-sm">
            <span className="text-muted-foreground">Spread:</span>
            <span className="font-mono font-semibold text-spread">
              {spread.toFixed(5)} ({spreadPercentage.toFixed(3)}%)
            </span>
          </div>
        </div>
      </div>

      {/* Column Headers */}
      <div className="grid grid-cols-3 gap-2 border-b border-border bg-secondary px-4 py-2 text-xs font-semibold text-muted-foreground">
        <div>Price (USD)</div>
        <div className="text-right">Amount</div>
        <div className="text-right">Total</div>
      </div>

      <div className="grid md:grid-cols-2">
        {/* Bids (Buy Orders) */}
        <div className="border-r border-border">
          <div className="bg-secondary/50 px-4 py-2 text-xs font-semibold text-bid">
            BIDS
          </div>
          <div className="max-h-[500px] overflow-y-auto">
            {data.bid.map((order, index) => (
              <OrderbookRow
                key={`bid-${index}`}
                price={order.price_}
                quantity={order.qty_}
                maxQuantity={maxBidQty}
                type="bid"
              />
            ))}
          </div>
        </div>

        {/* Asks (Sell Orders) */}
        <div>
          <div className="bg-secondary/50 px-4 py-2 text-xs font-semibold text-ask">
            ASKS
          </div>
          <div className="max-h-[500px] overflow-y-auto">
            {data.asks.map((order, index) => (
              <OrderbookRow
                key={`ask-${index}`}
                price={order.price_}
                quantity={order.qty_}
                maxQuantity={maxAskQty}
                type="ask"
              />
            ))}
          </div>
        </div>
      </div>
    </Card>
  );
};
