"""Source-bound prior CER identity only; no numerical operation or fake dispatch."""
from .resume_historical_gate import required_digest
def require_resume_cer_lineage(lineage,rows,binding,dispatch):
 expected=binding.get('resume_cer_allowed_origin')
 if not isinstance(expected,dict)or not isinstance(lineage,dict):raise ValueError('explicit frozen resume origin required')
 if lineage!=expected:raise ValueError('resume lineage differs from source-bound allowed origin')
 request_digest = required_digest('WORLDSWITHIN_DIAGNOSTIC_ORIGIN_REQUEST_SHA256')
 input_binding_digest = required_digest('WORLDSWITHIN_DIAGNOSTIC_ORIGIN_INPUT_SHA256')
 if lineage.get('origin_request_sha256') != request_digest or lineage.get('origin_input_binding_sha256') != input_binding_digest:
  raise ValueError('wrong bound resume origin')
 if lineage.get('origin_run')!='fusion_resume_origin' or lineage.get('origin_failed_run_preserved')is not True or lineage.get('origin_fused_frames')!=0 or lineage.get('origin_CER_owned_calls')!=97:raise ValueError('failed origin with complete CER lineage required')
 if dispatch!={'owned_calls':0,'original_calls':0}:raise ValueError('resumed process must not execute or fabricate CER calls')
 def identity(values):
  out={}
  for row in values:
   plane=row['plane']
   if plane in out or plane not in ('depth','conf0','conf1','conf2'):raise ValueError('duplicate/unknown CER plane')
   out[plane]=(row['bytes'],row['sha256'])
  if len(out)!=4:raise ValueError('four unique CER planes required')
  return out
 if identity(rows)!=identity(binding['historical_cer_outputs'])or identity(rows)!=identity(lineage['CER_banks']):raise ValueError('current/historical/prior CER bytes differ')
 return True
