import { AreaChart, Area, Tooltip, CartesianGrid, ResponsiveContainer } from 'recharts';
import colors from 'client/styles/colors';
import { Card } from 'client/components/Form/Card';
import Row from 'client/components/Form/Row';
import { formatDate, formatNumber } from 'client/utils/format';

const cardStyles = `
.rank-average {
  text-align: center;
  font-size: 1.8rem;
  font-weight: bold;
}
.chart-container {
  margin-top: 1rem;
}
`;

const makeRankStats = (data: { date: string; rank: number }[]) => {
  const average = Math.round(data.reduce((acc, cur) => acc + cur.rank, 0) / data.length);
  const today = data[0].rank;
  const yesterday = data[1]?.rank;
  const percentageChange = yesterday ? ((today - yesterday) / yesterday) * 100 : null;
  return {
    average,
    percentageChange,
  };
};

const makeChartData = (data: { date: string; rank: number }[]) => {
  return data.map((d) => {
    return {
      date: d.date,
      uv: d.rank,
    };
  });
};

function Chart(chartData: { date: string; uv: number }[], data: any) {
  return (
    <ResponsiveContainer width="100%" height={100}>
      <AreaChart width={400} height={100} data={chartData}>
        <CartesianGrid
          strokeDasharray="4"
          strokeWidth={0.25}
          verticalPoints={[50, 100, 150, 200, 250, 300, 350]}
          horizontalPoints={[25, 50, 75]}
        />
        <Tooltip
          contentStyle={{ background: colors.background, color: colors.textColor, borderRadius: 4 }}
          labelFormatter={(value) => `Date : ${formatDate(data[value as number].date)}`}
          formatter={(value) => formatNumber(Number(value))}
        />
        <Area
          type="monotone"
          dataKey="uv"
          stroke={colors.primary}
          fillOpacity={1}
          name="Rank"
          fill={`color-mix(in srgb, ${colors.backgroundSunken} 63%, transparent)`}
        />
      </AreaChart>
    </ResponsiveContainer>
  );
}

const RankCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const data = props.data.ranks || [];
  const { average, percentageChange } = makeRankStats(data);
  const chartData = makeChartData(data);
  return (
    <Card heading={props.title} actionButtons={props.actionButtons} styles={cardStyles}>
      <div className="rank-average">{formatNumber(data[0].rank)}</div>
      {percentageChange != null && (
        <Row
          lbl="Change since Yesterday"
          val={`${formatNumber(percentageChange, { digits: 2, signDisplay: 'exceptZero' })}%`}
        />
      )}
      <Row lbl="Historical Average Rank" val={formatNumber(average)} />
      <div className="chart-container">{Chart(chartData, data)}</div>
    </Card>
  );
};

export default RankCard;
