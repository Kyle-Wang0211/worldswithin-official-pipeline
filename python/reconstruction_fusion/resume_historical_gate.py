"""Metadata-only originalA first-difference stop; no numerical operation."""
from pathlib import Path
import hashlib,json,os,re
SUFFIXES={'.d16':'retained_d16','.camera.f64le':'retained_camera','.json':'retained_metadata','.col.u8':'retained_color'}
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def required_digest(setting):
    value = os.environ.get(setting, '')
    if re.fullmatch(r'[0-9a-f]{64}', value) is None:
        raise ValueError(f'{setting} requires a lowercase SHA-256 digest')
    return value
class HistoricalOutputGate:
 def __init__(self,oracle_path,expected_sha,total_binding,report,save):
  self.report,self.save=report,save
  if digest(oracle_path)!=expected_sha:raise ValueError('historical oracle bytes changed')
  self.expected={};raw=json.loads(Path(oracle_path).read_text())
  origin_run = os.environ.get('WORLDSWITHIN_DIAGNOSTIC_ORACLE_RUN_ID', '')
  if not origin_run.strip() or raw['original_run'] != origin_run:
   raise ValueError('WORLDSWITHIN_DIAGNOSTIC_ORACLE_RUN_ID must match the oracle')
  total = os.environ.get('WORLDSWITHIN_DIAGNOSTIC_TOTAL_KEPT', '')
  if re.fullmatch(r'[1-9][0-9]*', total) is None:
   raise ValueError('WORLDSWITHIN_DIAGNOSTIC_TOTAL_KEPT requires a positive integer')
  self.expected_total = int(total)
  rows=raw['original_stage_oracles']['fusionPly']['outputs']
  for row in rows:
   path=row['path'];name=Path(path).name
   if path!='dense/retained/'+name or path in self.expected:raise ValueError('duplicate or escaping historical path')
   role='order'if name=='order.json'else next((v for k,v in SUFFIXES.items()if name.endswith(k)),None)
   if row['role']!=role or type(row['bytes'])is not int or row['bytes']<=0 or len(row['sha256'])!=64:raise ValueError('historical role/size/SHA invalid')
   self.expected[path]=row
  required={'dense/retained/order.json'}|{'dense/retained/'+f'{i:08d}'+suffix for i in range(97)for suffix in SUFFIXES}
  if set(self.expected)!=required:raise ValueError('all389 unique historical outputs required')
  receipt_digest = required_digest('WORLDSWITHIN_DIAGNOSTIC_TOTAL_RECEIPT_SHA256')
  if total_binding != {'value': self.expected_total, 'original_pipeline_receipt_sha256': receipt_digest}:
   raise ValueError('exact original total source binding required')
  report['historical_output_checks']={'oracle_sha256':expected_sha,'status':'AUTHENTICATED_BEFORE_FUSION','frames_checked':0,'historical_perframe_histogram':'NOT_ESTABLISHED','historical_perframe_counts':'NOT_ESTABLISHED','original_total_binding':total_binding}
 def difference(self,details):
  self.report.update(status='FIRST_DIFFERENCE',first_difference_stage='fusionPly',first_difference=details);self.save();raise RuntimeError('first historical Fusion output difference')
 def check_file(self,target,name,role):
  expected=self.expected['dense/retained/'+name];path=Path(target)/name
  if not path.is_file()or path.is_symlink():return self.difference({'path':expected['path'],'reason':'missing/nonregular output'})
  actual={'path':expected['path'],'role':role,'bytes':path.stat().st_size,'sha256':digest(path)}
  if any(actual[k]!=expected[k]for k in ('path','role','bytes','sha256')):self.difference({'expected':expected,'actual':actual})
 def check_frame(self,frame,target):
  if frame!=self.report['historical_output_checks']['frames_checked']:raise ValueError('frame gate order changed')
  wanted={f'{frame:08d}'+suffix for suffix in SUFFIXES};actual={p.name for p in Path(target).glob(f'{frame:08d}.*')}
  if actual!=wanted:self.difference({'frame':frame,'expected_paths':sorted(wanted),'actual_paths':sorted(actual)})
  for suffix,role in SUFFIXES.items():self.check_file(target,f'{frame:08d}'+suffix,role)
  self.report['historical_output_checks']['frames_checked']+=1
 def check_order_and_total(self,target,kept):
  if self.report['historical_output_checks']['frames_checked']!=97:raise ValueError('all97 frames required before order')
  self.check_file(target,'order.json','order')
  if int(kept)!=self.expected_total:self.difference({'expected_total_kept':self.expected_total,'actual_total_kept':int(kept)})
  self.report['historical_output_checks']['status']='EXACT_389_FILES_AND_ORIGINAL_TOTAL'
