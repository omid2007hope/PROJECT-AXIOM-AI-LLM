import type { Analyzer } from '../types';

// Warn when no CAA issue tag limits which CAs can issue certificates
const caaRecords: Analyzer = (d) => {
  const records: any[] = Array.isArray(d.records) ? d.records : [];
  const issue = records.filter((r) => r.tag === 'issue');
  if (!issue.length) {
    return [
      {
        severity: 'warning',
        title: records.length ? 'CAA records have no issue tag' : 'No CAA records',
        detail: 'Any certificate authority can issue certificates for this domain',
      },
    ];
  }
  const issuers = [...new Set(issue.map((r) => r.value.split(';')[0].trim()).filter(Boolean))];
  if (!issuers.length) {
    return [{ severity: 'info', title: 'CAA blocks all certificate issuance' }];
  }
  return [
    {
      severity: 'pass',
      title: `CAA limits issuance to ${issuers.length} CA(s)`,
      detail: issuers.join(', '),
    },
  ];
};

export default caaRecords;
