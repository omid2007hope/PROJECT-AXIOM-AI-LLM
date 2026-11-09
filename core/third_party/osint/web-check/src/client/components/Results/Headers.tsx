import { Card } from 'client/components/Form/Card';
import Row, { type RowProps } from 'client/components/Form/Row';
import type { ReactNode } from 'react';
import { LEAK_HEADERS } from 'client/analysis/rules/headers';

const HeadersCard = (props: {
  data: any;
  title: string;
  actionButtons: ReactNode;
}): JSX.Element => {
  const headers = props.data;
  return (
    <Card heading={props.title} styles="grid-row: span 2;" actionButtons={props.actionButtons}>
      {Object.keys(headers).map((header: string, index: number) => {
        const val = headers[header];
        const leak: RowProps = LEAK_HEADERS.includes(header)
          ? { status: 'info', term: 'server-disclosure' }
          : {};
        return (
          <Row
            key={`header-${index}`}
            lbl={header}
            val={Array.isArray(val) ? val.join(', ') : val}
            {...leak}
          />
        );
      })}
    </Card>
  );
};

export default HeadersCard;
