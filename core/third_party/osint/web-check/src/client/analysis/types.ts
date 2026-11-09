export type Severity = 'critical' | 'issue' | 'warning' | 'info' | 'pass';

// Every severity, most severe first
export const SEVERITIES: Severity[] = ['critical', 'issue', 'warning', 'info', 'pass'];

export interface Finding {
  cardId: string;
  severity: Severity;
  title: string;
  detail?: string;
}

// A finding before it's tied to a card
export type Result = Omit<Finding, 'cardId'>;

export type Analyzer = (data: any) => Result[];
