import { Routes, Route, Outlet } from 'react-router';

import Results from 'client/views/Results.tsx';
import NotFound from 'client/views/NotFound.tsx';

import ErrorBoundary from 'client/components/boundaries/PageError.tsx';
import GlobalStyles from './styles/globals.tsx';

const Layout = () => {
  return (
    <>
      <GlobalStyles />
      <Outlet />
    </>
  );
};

export default function App() {
  return (
    <ErrorBoundary>
      <Routes>
        <Route path="/check" element={<Layout />}>
          <Route path=":urlToScan" element={<Results />} />
          <Route path="*" element={<NotFound />} />
        </Route>
        <Route path="/:tool" element={<Layout />}>
          <Route path=":urlToScan" element={<Results />} />
        </Route>
      </Routes>
    </ErrorBoundary>
  );
}
