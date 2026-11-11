/*
 * Project Axiom — Built from scratch. Engineered for the future.
 * Copyright (c) 2026 Omid Teimory. All rights reserved.
 */

import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import App from './App.jsx'

createRoot(document.getElementById('root')).render(
  <StrictMode>
    <App />
  </StrictMode>,
)
