import { Button } from "./ui/button";

interface SymbolSelectorProps {
  symbols: string[];
  selectedSymbol: string;
  onSelectSymbol: (symbol: string) => void;
}

export const SymbolSelector = ({
  symbols,
  selectedSymbol,
  onSelectSymbol,
}: SymbolSelectorProps) => {
  return (
    <div className="flex flex-wrap gap-2">
      {symbols.map((symbol) => (
        <Button
          key={symbol}
          variant={selectedSymbol === symbol ? "default" : "secondary"}
          onClick={() => onSelectSymbol(symbol)}
          className="font-mono font-semibold"
        >
          {symbol}
        </Button>
      ))}
    </div>
  );
};
