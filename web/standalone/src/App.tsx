import { Route, Routes } from "react-router-dom";
import { VersionBadge } from "@/components/VersionBadge";
import { HomePage } from "@/pages/HomePage";
import { LibToolPage } from "@/pages/LibToolPage";
import { ProjectView } from "@/pages/ProjectView";
import { ToolPage } from "@/pages/ToolPage";

export default function App() {
  return (
    <>
      <Routes>
        <Route path="/" element={<HomePage />} />
        <Route path="/p/:project" element={<ProjectView />} />
        <Route path="/p/:project/:tool/*" element={<ToolPage />} />
        <Route path="/l/:lib/:tool" element={<LibToolPage />} />
      </Routes>
      {/* Version + source link, bottom-right on every route (home + editor). */}
      <VersionBadge />
    </>
  );
}
