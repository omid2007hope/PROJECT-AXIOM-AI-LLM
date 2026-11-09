import type { Analyzer, Result } from '../types';
import { daysUntil } from '../helpers';
import { formatNumber } from 'client/utils/format';

// A field's value, matching its name in any case, and ignoring numbers added to repeats
export const getField = (fields: Record<string, string> = {}, name: string) =>
  Object.entries(fields).find(([key]) => key.toLowerCase().replace(/\d+$/, '') === name)?.[1];

// A problem when the file has no Contact field
export const checkContact = (fields?: Record<string, string>): Result | null =>
  getField(fields, 'contact')
    ? null
    : {
        severity: 'issue',
        title: 'security.txt has no Contact field',
        detail: 'Contact is required, as it tells researchers how to report a vulnerability',
      };

// A problem with the Expires field, when it's missing, past, or over a year ahead
export const checkExpires = (fields?: Record<string, string>): Result | null => {
  const days = daysUntil(getField(fields, 'expires'));
  if (days === null) {
    return {
      severity: 'warning',
      title: 'security.txt has no valid Expires date',
      detail: 'Expires is required. Set it under a year ahead, like 2027-01-01T00:00:00Z',
    };
  }
  if (days < 0) {
    return {
      severity: 'warning',
      title: `security.txt expired ${formatNumber(-days)} day(s) ago`,
      detail: 'Researchers treat an expired file as out of date, so review it and move Expires on',
    };
  }
  if (days > 366) {
    return {
      severity: 'info',
      title: 'security.txt expires over a year ahead',
      detail: 'Keep Expires under a year ahead, so the file gets reviewed regularly',
    };
  }
  return null;
};

// Flag a missing security.txt, or one missing the Contact and Expires fields it needs
const securityTxt: Analyzer = (d) => {
  if (!d.isPresent) {
    return [
      {
        severity: 'warning',
        title: 'No security.txt published',
        detail: 'Add /.well-known/security.txt with disclosure contact info',
      },
    ];
  }
  const out: ReturnType<Analyzer> = [{ severity: 'pass', title: 'security.txt found' }];
  for (const check of [checkContact(d.fields), checkExpires(d.fields)]) {
    if (check) out.push(check);
  }
  if (!d.isPgpSigned) {
    out.push({
      severity: 'info',
      title: 'security.txt not PGP signed',
      detail: 'Sign the file to let researchers verify authenticity',
    });
  }
  return out;
};

export default securityTxt;
