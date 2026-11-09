import { Card } from 'client/components/Form/Card';
import Row, { ExpandableRow, Section } from 'client/components/Form/Row';
import { getPath } from 'client/utils/result-processor';
import { formatNumber } from 'client/utils/format';

const convertToDate = (dateString: string): string => {
  const [date, time] = dateString.split(' ');
  const [year, month, day] = date.split('-').map(Number);
  const [hour, minute, second] = time.split(':').map(Number);
  const dateObject = new Date(Date.UTC(year, month - 1, day, hour, minute, second));
  if (isNaN(dateObject.getTime())) {
    return dateString;
  }
  return dateObject.toString();
};

const MalwareCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const urlHaus = props.data.urlHaus || {};
  const phishTank = props.data.phishTank || {};
  const cloudmersive = props.data.cloudmersive || {};
  const safeBrowsing = props.data.safeBrowsing || {};
  const netApi = props.data.netApi || {};
  const phished = phishTank.url0?.in_database === 'true' && phishTank.url0?.valid === 'true';
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {!safeBrowsing.error && (
        <Row
          lbl="Google Safe Browsing"
          val={!safeBrowsing.unsafe}
          yes="Safe"
          no="Unsafe"
          term="safe-browsing"
        />
      )}
      {typeof cloudmersive.CleanResult === 'boolean' && (
        <Row
          lbl="Cloudmersive"
          val={cloudmersive.CleanResult}
          yes="Safe"
          no="Unsafe"
          term="threat-scan"
        />
      )}
      {(!cloudmersive.error || safeBrowsing.details) && (
        <Row
          lbl="Threat Type"
          val={safeBrowsing.details?.[0]?.threatType || cloudmersive.WebsiteThreatType || 'None'}
        />
      )}
      {!phishTank.error && (
        <Row
          lbl="Phishing Status"
          val={!phished}
          yes="No Phishing Found"
          no="Phishing Identified"
          term="phishing"
        />
      )}
      {phishTank.url0?.valid === 'true' && phishTank.url0.phish_detail_page && (
        <Row
          lbl="Phish Info"
          val={phishTank.url0.phish_id}
          link={phishTank.url0.phish_detail_page}
        />
      )}
      {urlHaus.query_status === 'no_results' && (
        <Row lbl="Malware Status" val="No Malware Found" status="pass" term="malware" />
      )}
      {urlHaus.query_status === 'ok' && (
        <>
          <Row lbl="Status" val="Malware Identified" status="error" term="malware" />
          <Row lbl="First Seen" val={convertToDate(urlHaus.firstseen)} />
          <Row lbl="Bad URLs Count" val={formatNumber(urlHaus.url_count)} />
        </>
      )}
      {typeof netApi.listed_now === 'boolean' && (
        <Row
          lbl="NetAPI Compromised Feed"
          val={!netApi.listed_now}
          yes={netApi.listed_ever ? 'Not Listed Now' : 'Not Listed'}
          no="Listed"
          term="malware"
        />
      )}
      {netApi.listed_now && (
        <>
          <Row lbl="Listed Since" val={netApi.first_seen} />
          <Row lbl="Matched Entry" val={netApi.matched_value} />
          {netApi.block_reason && <Row lbl="Reason" val={netApi.block_reason} />}
        </>
      )}
      {urlHaus.urls && (
        <Section title="Reported URLs" collapsible>
          {urlHaus.urls.map((urlResult: any) => {
            const rows = [
              { lbl: 'ID', val: urlResult.id },
              { lbl: 'Status', val: urlResult.url_status },
              { lbl: 'Date Added', val: convertToDate(urlResult.date_added) },
              { lbl: 'Threat Type', val: urlResult.threat },
              { lbl: 'Reported By', val: urlResult.reporter },
              {
                lbl: 'Takedown Time',
                val:
                  urlResult.takedown_time_seconds &&
                  `${formatNumber(urlResult.takedown_time_seconds)} seconds`,
              },
              { lbl: 'Larted', val: urlResult.larted },
              { lbl: 'Tags', val: (urlResult.tags || []).join(', ') },
              { lbl: 'Reference', val: urlResult.urlhaus_reference },
              { lbl: 'File Path', val: urlResult.url },
            ];
            return (
              <ExpandableRow
                key={urlResult.id}
                lbl={`${getPath(urlResult.url)} (${urlResult.id})`}
                rowList={rows}
              />
            );
          })}
        </Section>
      )}
    </Card>
  );
};

export default MalwareCard;
