import { useState, useEffect } from "react";
import { OrderbookDisplay } from "@/components/OrderbookDisplay";
import { SymbolSelector } from "@/components/SymbolSelector";
import { useToast } from "@/hooks/use-toast";

interface Order {
  price_: number;
  qty_: number;
}

interface Book {
  symbol: string;
  bid: Order[];
  asks: Order[];
}

interface ApiResponse {
  books: Book[];
}

const Index = () => {
  const [orderbookData, setOrderbookData] = useState<ApiResponse | null>(null);
  const [selectedSymbol, setSelectedSymbol] = useState<string>("XRP/USD");
  const [isLoading, setIsLoading] = useState(true);
  const { toast } = useToast();

  const fetchOrderbook = async () => {
    try {
      // Replace this URL with your actual API endpoint
      const response = await fetch("http://127.0.0.1:4000/book");
      const data: ApiResponse = await response.json();
      setOrderbookData(data);
      setIsLoading(false);
    } catch (error) {
      console.error("Failed to fetch orderbook data:", error);
      toast({
        title: "Error",
        description: "Failed to fetch orderbook data",
        variant: "destructive",
      });

      // Fallback to mock data for demo purposes
      const mockData: ApiResponse = {
        books: [
          {
            symbol: "XRP/USD",
            bid: [
              { price_: 2.34309, qty_: 4088 },
              { price_: 2.34189, qty_: 396.5625 },
              { price_: 2.34174, qty_: 12365.45436627 },
              { price_: 2.34165, qty_: 396.5625 },
              { price_: 2.34163, qty_: 22.03070257 },
              { price_: 2.34162, qty_: 4270.53237653 },
              { price_: 2.34155, qty_: 4270.66601497 },
              { price_: 2.34149, qty_: 53.7 },
              { price_: 2.34145, qty_: 4270.85092144 },
              { price_: 2.34131, qty_: 2767.74764954 },
            ],
            asks: [
              { price_: 2.3431, qty_: 23725.750487190002 },
              { price_: 2.34318, qty_: 16110.670930119999 },
              { price_: 2.34319, qty_: 2401.964191 },
              { price_: 2.34322, qty_: 8535.29777466 },
              { price_: 2.34324, qty_: 8535.1905908 },
              { price_: 2.34334, qty_: 396.5625 },
              { price_: 2.3434, qty_: 4605.7985317 },
              { price_: 2.34343, qty_: 1281.53100237 },
              { price_: 2.34351, qty_: 644.7417798600001 },
              { price_: 2.34352, qty_: 8961.5 },
            ],
          },
          {
            symbol: "ETH/USD",
            bid: [
              { price_: 3639.09, qty_: 6.86508396 },
              { price_: 3639.03, qty_: 4.29593636 },
              { price_: 3638.98, qty_: 2.74802105 },
              { price_: 3638.82, qty_: 0.23 },
              { price_: 3638.81, qty_: 2.74814633 },
              { price_: 3638.79, qty_: 2.74816199 },
              { price_: 3638.7, qty_: 5.34399533 },
              { price_: 3638.59, qty_: 2.7483125 },
              { price_: 3638.5, qty_: 0.82432642 },
              { price_: 3638.43, qty_: 0.00725924 },
            ],
            asks: [
              { price_: 3639.1, qty_: 7.37354367 },
              { price_: 3639.11, qty_: 0.16 },
              { price_: 3639.47, qty_: 17.3207 },
              { price_: 3639.63, qty_: 9.92478142 },
              { price_: 3639.79, qty_: 0.027 },
              { price_: 3639.8, qty_: 19.223 },
              { price_: 3639.9, qty_: 5.36925698 },
              { price_: 3639.98, qty_: 1.37363734 },
              { price_: 3640, qty_: 0.8437 },
              { price_: 3640.03, qty_: 2.7472327 },
            ],
          },
          {
            symbol: "BTC/USD",
            bid: [
              { price_: 106856.6, qty_: 0.03204447 },
              { price_: 106840, qty_: 0.00006083 },
              { price_: 106830, qty_: 0.00006084 },
              { price_: 106820, qty_: 0.00006085 },
              { price_: 106813.1, qty_: 0.00936194 },
              { price_: 106811.8, qty_: 0.01190442 },
              { price_: 106811.7, qty_: 0.09362268 },
              { price_: 106810.4, qty_: 0.04680971 },
              { price_: 106810.2, qty_: 0.09362395 },
              { price_: 106810, qty_: 0.00006085 },
            ],
            asks: [
              { price_: 106856.7, qty_: 8.733147409999997 },
              { price_: 106856.9, qty_: 0.09731722 },
              { price_: 106857.6, qty_: 1.90036451 },
              { price_: 106857.7, qty_: 0.20117 },
              { price_: 106857.9, qty_: 1.8292989 },
              { price_: 106858.5, qty_: 0.2150002 },
              { price_: 106859.1, qty_: 0.0935812 },
              { price_: 106860, qty_: 0.00025 },
              { price_: 106860.1, qty_: 0.00006083 },
              { price_: 106861, qty_: 0.09357954 },
            ],
          },
        ],
      };
      setOrderbookData(mockData);
      setIsLoading(false);
    }
  };

  useEffect(() => {
    fetchOrderbook();
    const interval = setInterval(fetchOrderbook, 100); // Poll every 2 seconds
    return () => clearInterval(interval);
  }, []);

  const currentBook = orderbookData?.books.find(
    (book) => book.symbol === selectedSymbol,
  );

  const availableSymbols =
    orderbookData?.books.map((book) => book.symbol) || [];

  if (isLoading) {
    return (
      <div className="flex min-h-screen items-center justify-center bg-background">
        <div className="text-center">
          <div className="mb-4 h-12 w-12 animate-spin rounded-full border-4 border-primary border-t-transparent mx-auto" />
          <p className="text-muted-foreground">Loading orderbook data...</p>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-background p-4 md:p-8">
      <div className="mx-auto max-w-7xl space-y-6">
        {/* Header */}
        <div className="space-y-2">
          <h1 className="text-3xl font-bold">L2 Orderbook</h1>
          <p className="text-muted-foreground">
            Real-time cryptocurrency orderbook data with live updates
          </p>
        </div>

        {/* Symbol Selector */}
        <SymbolSelector
          symbols={availableSymbols}
          selectedSymbol={selectedSymbol}
          onSelectSymbol={setSelectedSymbol}
        />

        {/* Orderbook Display */}
        {currentBook && <OrderbookDisplay data={currentBook} />}
      </div>
    </div>
  );
};

export default Index;
